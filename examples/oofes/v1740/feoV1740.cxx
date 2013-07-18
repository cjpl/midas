/****************************************************************************/
/**
\file feoV1740.cxx

\mainpage

## Contents

Standard Midas Frontend for Optical access to the CAEN v1740 using the A3818 CONET2 driver

## Notes about this example

This example has been designed so that it should compile and work by default
without actual actual access to v1740 hardware. We have turned off portions
of code which make use of the driver to the actual hardware.  Where data
acquisition should be performed, we generate random data instead
(see v1740CONET2::ReadEvent).  See usage below to use real hardware.

The setup assumed in this example is the following:
- 1 frontend
- 1 optical link per frontend
- Arbitrary number of v1740 modules connected to each optical link (daisy-chained)

For a more complicated setup (many modules per frontend, or many frontends
using the event building mechanism), see the v1720 example

## Usage

Simulation:
Simply set the Nv1740 macro below to the number of boards you wish to simulate
and run "make SIMULATION=1".  Run ./feoV1740.exe

Real hardware:
Set the Nv1740 macro below to the number of boards that should be controlled by
the frontend on a specific optical link.  The frontend can be assigned an index
number (ie ./frontend -i 0), and this will be assumed to be the optical link
number to communicate with, after which all boards on that link will be started.
If no index number is assigned, the frontend will communicate with link 0 and
start all the boards on that link.


Compile using "make SIMULATION=0".  Run ./feoV1740.exe
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <vector>
using std::vector;

#include "midas.h"
#include "mvmestd.h"

extern "C" {
#include "CAENComm.h"
}

#include "v1740CONET2.hxx"

using namespace std;

#define Nv1740    1 //!< Set the number of v1720 modules to be used
#define EQ_EVID   4 //!< Event ID
#define EQ_TRGMSK 0 //!< Trigger mask

#if SIMULATION
#define FE_NAME "feoV1740_SIM"  //!< Frontend name (simulation)
#else
#define FE_NAME   "feoV1740"    //!< Frontend name
#endif

#define UNUSED(x) ((void)(x))   //!< Suppress compiler warnings

/* Globals */
/* Hardware */
extern HNDLE hDB;   //!< main ODB handle
extern BOOL debug;  //!< debug printouts

/* make frontend functions callable from the C framework */
#ifdef __cplusplus
extern "C" {
#endif

/*-- Globals -------------------------------------------------------*/

//! The frontend name (client name) as seen by other MIDAS clients
char *frontend_name = (char*)FE_NAME;
//! The frontend file name, don't change it
char *frontend_file_name = (char*)__FILE__;
//! frontend_loop is called periodically if this variable is TRUE
BOOL frontend_call_loop = FALSE;
//! a frontend status page is displayed with this frequency in ms
INT display_period = 000;
//! maximum event size produced by this frontend
INT max_event_size = 32 * 34000;
//! maximum event size for fragmented events (EQ_FRAGMENTED)
INT max_event_size_frag = 5 * 1024 * 1024;
//! buffer size to hold events
INT event_buffer_size = 200 * max_event_size;

bool runInProgress = false; //!< run is in progress
bool runOver = false; //!< run is over
bool runStopRequested = false; //!< stop run requested

/*-- Function declarations -----------------------------------------*/
INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);  //!< Interrupt Service Routine

INT read_trigger_event(char *pevent, INT off);
INT read_scaler_event(char *pevent, INT off);

/*-- Equipment list ------------------------------------------------*/
#undef USE_INT
//! Main structure for midas equipment
EQUIPMENT equipment[] =
{
  {
#if SIMULATION
    "FEv1740_SIM",              /* equipment name */
#else
    "FEv1740",                  /* equipment name */
#endif
    {
      EQ_EVID, EQ_TRGMSK,       /* event ID, trigger mask */
      "SYSTEM",                 /* event buffer */

#ifdef USE_INT
      EQ_INTERRUPT,             /* equipment type */
#else
      EQ_POLLED,                /* equipment type */
#endif

      LAM_SOURCE(0, 0x0),       /* event source crate 0, all stations */
      "MIDAS",                  /* format */
      TRUE,                     /* enabled */
      RO_RUNNING,               /* read only when running */
      500,                      /* poll for 500ms */
      0,                        /* stop run after this event limit */
      0,                        /* number of sub events */
      0,                        /* don't log history */
      "", "", ""
    },
    read_trigger_event,         /* readout routine */
  },
  {""}
};

#ifdef __cplusplus
}
#endif

// v1740 object
vector<v1740CONET2> ov1740; //!< objects for the v1740 modules controlled by this frontend
vector<v1740CONET2>::iterator itv1740; //!< iterator


/********************************************************************/
/********************************************************************/
/********************************************************************/

/**
 * \brief   Sequencer callback info
 *
 * Function which gets called when record is updated
 *
 * \param   [in]  hDB main ODB handle
 * \param   [in]  hseq Handle for key where search starts in ODB, zero for root.
 * \param   [in]  info Record descriptor additional info
 */
void seq_callback(INT hDB, INT hseq, void *info)
{
  KEY key;

  for (itv1740 = ov1740.begin(); itv1740 != ov1740.end(); ++itv1740) {
    if (hseq == itv1740->GetODBHandle()) {
      db_get_key(hDB, hseq, &key);
      itv1740->SetSettingsTouched(true);
      printf("Settings %s touched. Changes will take effect at start of next run.\n", key.name);
    }
  }
}

/**
 * \brief   Frontend initialization
 *
 * Runs once at application startup.  We initialize the hardware and optical
 * interfaces and set the equipment status in ODB.  We also lock the frontend
 *  to once physical cpu core.
 *
 * \return  Midas status code
 */
INT frontend_init()
{
  set_equipment_status(equipment[0].name, "Initializing...", "#FFFF00");

  // Suppress watchdog for PCIe for now
  cm_set_watchdog_params(FALSE, 0);

  // --- Get the frontend index. Derive the Optical link number
  int feIndex = get_frontend_index();

  int tNActivev1740=0;  //Number of v1740 boards activated at the end of frontend_init

  for (int iBoard=0; iBoard < Nv1740; iBoard++)
  {
    printf("<<< Init board %i\n", iBoard);

    //If no index supplied, use link 0, else use index as link number
    if(feIndex == -1)
      ov1740.push_back(v1740CONET2(0, iBoard, iBoard));
    else
      ov1740.push_back(v1740CONET2(feIndex, iBoard, iBoard));

    ov1740.back().verbose = 1;

    //load ODB settings
    ov1740.back().SetODBRecord(hDB,seq_callback);

    // Open Optical interface
    CAENComm_ErrorCode sCAEN;
    sCAEN = ov1740.back().Connect();
    if (sCAEN != CAENComm_Success) {
      cm_msg(MERROR, "fe", "Could not connect to board; error:%d", sCAEN);
    }
    else {
      tNActivev1740++;
      printf("Board#:%d Module_Handle[%d]:%d (active:%d)\n",
          iBoard, iBoard, ov1740.back().GetHandle(), tNActivev1740);

      ov1740.back().CheckBoardType();
    }
  }

  printf(">>> End of Init. %d active v1740. Expected %d\n\n", tNActivev1740, Nv1740);

  set_equipment_status(equipment[0].name, "Initialized", "#00ff00");
  printf("end of Init: %d\n", SUCCESS);

#if SIMULATION
  printf("*** RUNNING SIMULATION ***\n");
#endif

  return SUCCESS;
}

/**
 * \brief   Frontend exit
 *
 * Runs at frontend shutdown.  Disconnect hardware and set equipment status in ODB
 *
 * \return  Midas status code
 */
INT frontend_exit()
{ 
  set_equipment_status(equipment[0].name, "Exiting...", "#FFFF00");
  cout << "frontend_exit";

  for (itv1740 = ov1740.begin(); itv1740 != ov1740.end(); ++itv1740) {
    if (itv1740->IsConnected()){
      cm_msg(MINFO,"exit", "Closing handle %d", itv1740->GetHandle());
      itv1740->Disconnect();
    }
  }

  set_equipment_status(equipment[0].name, "Exited", "#00ff00");
  printf("End of exit\n");
  return SUCCESS;
}

/**
 * \brief   Begin of Run
 *
 * Called every run start transition.  Set equipment status in ODB,
 * start acquisition on the modules.
 *
 * \param   [in]  run_number Number of the run being started
 * \param   [out] error Can be used to write a message string to midas.log
 */
INT begin_of_run(INT run_number, char *error)
{

  set_equipment_status(equipment[0].name, "Starting run...", "#FFFF00");

  printf("<<< Begin of begin_of_run\n");

  CAENComm_ErrorCode sCAEN = CAENComm_Success;  //hardcoded for now

  for (itv1740 = ov1740.begin(); itv1740 != ov1740.end(); ++itv1740) {
    if (! itv1740->IsConnected()) continue;   // Skip unconnected board

    //  //Done at SetOdbRecord (frontend_init)
    //  size = sizeof(V1740_CONFIG_SETTINGS);
    //  if ((status = db_get_record (hDB, hSet, &tsvc, &size, 0)) != DB_SUCCESS)
    //    return status;

    itv1740->InitializeForAcq();
    itv1740->StartRun();
  }

  runInProgress = true;

  //------ FINAL ACTIONS before BOR -----------
  set_equipment_status(equipment[0].name, "Started run", "#00ff00");
  printf(">>> End of begin_of_run\n\n");
  return (sCAEN == CAENComm_Success ? SUCCESS : sCAEN);
}

/**
 * \brief   End of Run
 *
 * Called every stop run transition. Set equipment status in ODB,
 * stop acquisition on the modules.
 *
 * \param   [in]  run_number Number of the run being ended
 * \param   [out] error Can be used to write a message string to midas.log
 */
INT end_of_run(INT run_number, char *error)
{
  set_equipment_status(equipment[0].name, "Ending run...", "#FFFF00");

  printf("<<< Begin of end_of_run \n");
  // Stop run
  CAENComm_ErrorCode sCAEN = CAENComm_Success;
  uint32_t status, eStored;

  for (itv1740 = ov1740.begin(); itv1740 != ov1740.end(); ++itv1740) {
    if (itv1740->IsConnected()) {  // Skip unconnected board
      sCAEN = itv1740->StopRun();

      itv1740->ReadReg(V1740_ACQUISITION_STATUS, &status);
      itv1740->ReadReg(V1740_EVENT_STORED, &eStored);

      cout << "End of EOR. handle=" << itv1740->GetHandle() << " link=" << itv1740->GetLink()
                                   << " ACQ_STATUS=" << status << " EV_STORED=" << eStored << " sCAEN=" << sCAEN << endl;
    }
  }

  // Stop DAQ for seting up the parameters
  runOver = false;
  runStopRequested = false;
  runInProgress = false;

  printf(">>> End Of end_of_run\n\n");
  set_equipment_status(equipment[0].name, "Ended run", "#00ff00");

  return (sCAEN == CAENComm_Success ? SUCCESS : sCAEN);
}

/**
 * \brief   Pause Run
 *
 * Called every pause run transition.
 *
 * \param   [in]  run_number Number of the run being ended
 * \param   [out] error Can be used to write a message string to midas.log
 *
 * \return  Midas status code
 */
INT pause_run(INT run_number, char *error)
{
  cout << "pause";

  runInProgress = false;
  return SUCCESS;
}

/**
 * \brief   Resume Run
 *
 * Called every resume run transition.
 *
 * \param   [in]  run_number Number of the run being ended
 * \param   [out] error Can be used to write a message string to midas.log
 *
 * \return  Midas status code
 */
INT resume_run(INT run_number, char *error)
{
  cout << "resume";
  runInProgress = true;
  return SUCCESS;
}

/**
 * \brief   Frontend loop
 *
 * If frontend_call_loop is true, this routine gets called when
 * the frontend is idle or once between every event.
 *
 * \return  Midas status code
 */
INT frontend_loop()
{
  cout << "frontend_lopp";
  /* if frontend_call_loop is true, this routine gets called when
     the frontend is idle or once between every event */
  char str[128];
  static DWORD evlimit;

  if (runStopRequested && !runOver) {
    db_set_value(hDB,0,"/logger/channels/0/Settings/Event limit", &evlimit, sizeof(evlimit), 1, TID_DWORD);
    if (cm_transition(TR_STOP, 0, str, sizeof(str), ASYNC, FALSE) != CM_SUCCESS) {
      cm_msg(MERROR, "feodeap", "cannot stop run: %s", str);
    }
    runInProgress = false;
    runOver = true;
    cm_msg(MERROR, "feodeap","feodeap Stop requested");
  }
  return SUCCESS;
}

/*------------------------------------------------------------------*/
/********************************************************************\
  Readout routines for different events
\********************************************************************/
int Nloop;  //!< Number of loops executed in event polling
int Ncount; //!< Loop count for event polling timeout

// ___________________________________________________________________
/*-- Trigger event routines ----------------------------------------*/
/**
 * \brief   Polling routine for events.
 *
 * \param   [in]  source Event source (LAM/IRQ)
 * \param   [in]  count Loop count for event polling timeout
 * \param   [in]  test flag used to time the polling
 * \return  1 if event is available, 0 if done polling (no event).
 * If test equals TRUE, don't return.
 */
extern "C" INT poll_event(INT source, INT count, BOOL test)
/* Polling routine for events. Returns TRUE if event
   is available. If test equals TRUE, don't return. The test
   flag is used to time the polling */
{

  register int i;  // , mod=-1;
  register uint32_t lam = 0;
  //  register uint32_t event_size = 0;
  CAENComm_ErrorCode sCAEN;

  for (i=0; i<count; i++)
  {
    sCAEN = ov1740[0].ReadReg(V1740_ACQUISITION_STATUS, &lam);
    UNUSED(sCAEN);
    lam &= 0x8;

    if (lam) {
      Nloop = i; Ncount = count;
      if (!test)
        return lam;
    }
  }

  return 0;
}

/**
 * \brief   Interrupt configuration (not implemented)
 *
 * Routine for interrupt configuration if equipment is set in EQ_INTERRUPT
 * mode.  Not implemented right now, returns SUCCESS.
 *
 * \param   [in]  cmd Command for interrupt events (see midas.h)
 * \param   [in]  source Equipment index number
 * \param   [in]  adr Interrupt routine (see mfe.c)
 *
 * \return  Midas status code
 */
extern "C" INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
  cout << "interrupt_configure";
  switch (cmd) {
  case CMD_INTERRUPT_ENABLE:
    break;
  case CMD_INTERRUPT_DISABLE:
    break;
  case CMD_INTERRUPT_ATTACH:
    break;
  case CMD_INTERRUPT_DETACH:
    break;
  }
  return SUCCESS;
}

/**
 * \brief   Trigger event readout
 *
 * Main trigger event readout routine.  This is called by the polling or interrupt routines.
 * (see mfe.c).  For each module, read the event buffer into a midas data bank.
 *
 * \param   [in]  pevent Pointer to event buffer
 * \param   [in]  off Caller info (unused here), see mfe.c
 *
 * \return  Size of the event
 */
INT read_trigger_event(char *pevent, INT off)
{
  if (!runInProgress) return 0;

  DWORD *pdata;
  DWORD event_size, sn;
  int dwords_read;
  //  int sLoop, eEvent;

  sn = SERIAL_NUMBER(pevent);
  bk_init32(pevent);

  CAENComm_ErrorCode sCAEN;

  for (itv1740 = ov1740.begin(); itv1740 != ov1740.end(); ++itv1740) {
    if (!itv1740->IsConnected()) continue;   // Skip unconnected board

    itv1740->ReadReg(V1740_EVENT_SIZE, &event_size);
    if (event_size > 0)
    {

      char bankName[5];
      sprintf(bankName,"W4%02d",itv1740->GetLink());
      bk_create(pevent, bankName, TID_DWORD, &pdata);

      sCAEN = itv1740->ReadEvent(pdata, &dwords_read);

      //(eStored > 1) ? sLoop = 2 : sLoop = eStored;
      //  for (eEvent=0; eEvent < sLoop; eEvent++) {
      //sCAEN = CAENComm_Read32(v1740->GetHandle(), V1740_EVENT_SIZE, &eSize);

      bk_close(pevent, pdata + dwords_read);
    }
  }

  //primitive progress bar
  if (sn % 100 == 0) printf(".");

  UNUSED(sCAEN);
  return bk_size(pevent);
}

/**
 * \brief   Scaler event readout
 *
 * Scaler event readout routine.  Not used in this example.
 *
 * \param   [in]  pevent Pointer to event buffer
 * \param   [in]  off Caller info (unused here), see mfe.c
 *
 * \return  Size of the event
 */
INT read_scaler_event(char *pevent, INT off)
{
  /* init bank structure */
  bk_init(pevent);
  return 0;  // bk_size(pevent);
}
