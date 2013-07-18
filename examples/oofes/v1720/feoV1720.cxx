/*****************************************************************************/
/**
\file feoV1720.cxx

\mainpage

## Contents

Standard Midas Frontend for Optical access to the CAEN v1720 using the A3818 CONET2 driver

## File organization

- Compile time parameters setting
- MIDAS global variable defintion
- MIDAS function declaration
- Equipment variables
- functions

## Callback routines for system transitions

These routines are called whenever a system transition like start/
stop of a run occurs. The routines are called on the following
occations:

- frontend_init:  When the frontend program is started. This routine
                should initialize the hardware.

- frontend_exit:  When the frontend program is shut down. Can be used
                to releas any locked resources like memory, commu-
                nications ports etc.

- begin_of_run:   When a new run is started. Clear scalers, open
                rungates, etc.

- end_of_run:     Called on a request to stop a run. Can send
                end-of-run event and close run gates.

- pause_run:      When a run is paused. Should disable trigger events.

- resume_run:     When a run is resumed. Should enable trigger events.

## Notes about this example

This example has been designed so that it should compile and work
by default without actual actual access to v1720 hardware. We have turned
off portions of code which make use of the driver to the actual hardware.
Where data acquisition should be performed, we generate random data instead
(see v1720CONET2::ReadEvent()). See usage below to use real hardware.

The simulation code assumes the following setup:
- 1 frontend only
- Arbitrary number of v1720 modules
- Event builder not used

The code to use real hardware assumes this setup:
- 1 A3818 PCI-e board per PC to receive optical connections
- NBLINKSPERA3818 links per A3818 board
- NBLINKSPERFE optical links controlled by each frontend
- NB1720PERLINK v1720 modules per optical link (daisy chained)
- NBV1720TOTAL v1720 modules in total
- The event builder mechanism is used

## Usage

### Simulation
Simply set the Nv1720 macro below to the number of boards you wish to simulate,
compile and run:
    make SIMULATION=1
    ./feoV1720.exe

### Real hardware
Adjust NBLINKSPERA3818, NBLINKSPERFE, NB1720PERLINK and NBV1720TOTAL below according
to your setup.  NBV1720TOTAL / (NBLINKSPERFE * NB1720PERLINK) frontends
must be started in total. When a frontend is started, it must be assigned an index
number:

    ./frontend -i 0

If no index number is supplied, it is assumed that only 1 frontend is used to control
all boards on all links on that computer.

For example, consider the following setup:

    NBLINKSPERA3818    4     // Number of optical links used per A3818
    NBLINKSPERFE       2     // Number of optical links controlled by each frontend
    NB1720PERLINK      2     // Number of daisy-chained v1720s per optical link
    NBV1720TOTAL       32    // Number of v1720 boards in total

We will need 32/(2*2) = 8 frontends (8 indexes; from 0 to 7).  Each frontend
controls 2*2 = 4 v1720 boards.  Compile and run:

    make SIMULATION=0
    ./feoV1720.exe

$Id: feov1720.cxx 128 2011-05-12 06:26:34Z alex $
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/resource.h>

#include <vector>
using std::vector;

#include "midas.h"

extern "C" {
#include "CAENComm.h"
#include "ov1720drv.h"
}

#include "v1720CONET2.hxx"

// __________________________________________________________________
// --- General feov1720 parameters

#if SIMULATION
#define Nv1720 2              //!< Set the number of v1720 modules to be used
#else
#define NBLINKSPERA3818   1   //!< Number of optical links used per A3818
#define NBLINKSPERFE      1   //!< Number of optical links controlled by each frontend
#define NB1720PERLINK     2   //!< Number of daisy-chained v1720s per optical link
#define NBV1720TOTAL      2  //!< Number of v1720 boards in total
#endif

#define  EQ_EVID   1                //!< Event ID
#define  EQ_TRGMSK 0                //!< Trigger mask
#if SIMULATION
#define  FE_NAME   "feov1720_SIM"   //!< Frontend name (simulation)
#else
#define  FE_NAME   "feov1720"       //!< Frontend name
#endif

#define UNUSED(x) ((void)(x)) //!< Suppress compiler warnings

// __________________________________________________________________
// --- MIDAS global variables
extern HNDLE hDB;   //!< main ODB handle
extern BOOL debug;  //!< debug printouts

BOOL debugpolling = false;  //!< debug msgs polling loop
BOOL debugtrigger = false;  //!< debug msgs read trigger event

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
INT event_buffer_size = 200 * max_event_size + 10000;

bool runInProgress = false; //!< run is in progress
bool runOver = false; //!< run is over
bool runStopRequested = false; //!< stop run requested


// __________________________________________________________________
/*-- MIDAS Function declarations -----------------------------------------*/
INT frontend_init();
INT frontend_exit();
INT begin_of_run(INT run_number, char *error);
INT end_of_run(INT run_number, char *error);
INT pause_run(INT run_number, char *error);
INT resume_run(INT run_number, char *error);
INT frontend_loop();
extern void interrupt_routine(void);  //!< Interrupt Service Routine

INT read_trigger_event(char *pevent, INT off);
INT read_scaler_event(char *pevent, INT off); //!< Placeholder

// __________________________________________________________________
/*-- Equipment list ------------------------------------------------*/
#undef USE_INT
//! Main structure for midas equipment
EQUIPMENT equipment[] =
{
  {
#if SIMULATION
    "FEv1720_SIM",            /* equipment name */
    {
      EQ_EVID, EQ_TRGMSK,     /* event ID, trigger mask */
      "SYSTEM",               /* event buffer */
# ifdef USE_INT
      EQ_INTERRUPT,           /* equipment type */
# else
      EQ_POLLED,              /* equipment type */
# endif //USE_INT

#else
    "FEv1720I%0d",            /* equipment name */
    {
      EQ_EVID, EQ_TRGMSK,     /* event ID, trigger mask */
      //      //Use this to make different frontends (indexes) write
      //      //to different buffers
      //      "BUF%0d",               /* event buffer */
      "SYSTEM",               /* event buffer */

# ifdef USE_INT
      EQ_INTERRUPT,           /* equipment type */
# else
      EQ_POLLED | EQ_EB,      /* equipment type */
# endif //USE_INT

#endif //SIMULATION

      LAM_SOURCE(0, 0x0),     /* event source crate 0, all stations */
      "MIDAS",                /* format */
      TRUE,                   /* enabled */
      RO_RUNNING,             /* read only when running */
      500,                    /* poll for 500ms */
      0,                      /* stop run after this event limit */
      0,                      /* number of sub events */
      0,                      /* don't log history */
      "", "", ""
    },
    read_trigger_event,       /* readout routine */
  },
  {""}
};

#ifdef __cplusplus
}
#endif

vector<v1720CONET2> ov1720; //!< objects for the v1720 modules controlled by this frontend
vector<v1720CONET2>::iterator itv1720;  //!< iterator


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
void seq_callback(INT hDB, INT hseq, void *info){
  KEY key;

  for (itv1720 = ov1720.begin(); itv1720 != ov1720.end(); ++itv1720) {
    if (hseq == itv1720->GetODBHandle()){
      db_get_key(hDB, hseq, &key);
      itv1720->SetSettingsTouched(true);
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
INT frontend_init(){

  set_equipment_status(equipment[0].name, "Initializing...", "#FFFF00");

  // --- Suppress watchdog for PICe for now
  cm_set_watchdog_params(FALSE, 0);

#if SIMULATION
  for (int iBoard=0; iBoard < Nv1720; iBoard++)
  {
    printf("<<< Init board %i\n", iBoard);
    ov1720.push_back(v1720CONET2(iBoard));
    ov1720[iBoard].verbose = 1;

    //load ODB settings
    ov1720[iBoard].SetODBRecord(hDB,seq_callback);

    // Open Optical interface
    CAENComm_ErrorCode sCAEN;
    sCAEN = ov1720[iBoard].Connect();
    if (sCAEN != CAENComm_Success) {
      cm_msg(MERROR, "fe", "Could not connect to board; error:%d", sCAEN);
    }
    else {
      printf("Connected successfully to module; handle: %d\n",ov1720[iBoard].GetHandle());

      ov1720[iBoard].InitializeForAcq();
      ov1720[iBoard].mZLE = false;
    }
  }

  // Lock to one core on the processor. Need to be moved to TFeCommon
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(4, &mask);  // arbitrarily chose 4
  if( sched_setaffinity(0, sizeof(mask), &mask) < 0 ){
    printf("ERROR: affinity not set\n");
  }
  printf(">>> End of Init.\n\n");
#else
  // --- Get the frontend index. Derive the Optical link number
  int feIndex = get_frontend_index();

  int tNActivev1720=0;  //Number of v1720 boards activated at the end of frontend_init

  // If no index was supplied on the command-line, assume 1 frontend
  // to control all the links and boards
  if(feIndex == -1){

    printf("<<< No index supplied! Assuming only 1 frontend only and starting all boards on all links\n");
    for (int iLink=0; iLink < NBLINKSPERA3818; iLink++)
    {
      for (int iBoard=0; iBoard < NB1720PERLINK; iBoard++)
      {
        printf("<<< Begin of Init \n link:%i, board:%i\n", iLink, iBoard);

        // Compose unique module ID
        int moduleID = iLink*NB1720PERLINK + iBoard;

        // Create module objects
        ov1720.push_back(v1720CONET2(feIndex, iLink, iBoard, moduleID));
        ov1720.back().verbose = 1;

        // Setup ODB record (create if necessary)
        ov1720.back().SetODBRecord(hDB,seq_callback);

        // Open Optical interface
        printf("Opening optical interface Link %d, Board %d\n", iLink, iBoard);
        CAENComm_ErrorCode sCAEN;
        sCAEN = ov1720.back().Connect();
        if (sCAEN != CAENComm_Success) {
          cm_msg(MERROR, "fe", "Link#:%d iBoard#:%d error:%d", iLink, iBoard, sCAEN);
        }
        else {
          tNActivev1720++;
          printf("Link#:%d Board#:%d Module_Handle[%d]:%d (active:%d)\n",
              iLink, iBoard, moduleID, ov1720.back().GetHandle(), tNActivev1720);

          ov1720.back().InitializeForAcq();

        }
      }
    }
  }
  else {  //index supplied

    if((NBV1720TOTAL % (NB1720PERLINK*NBLINKSPERFE)) != 0){
      printf("Incorrect setup: the number of boards controlled by each frontend"
          " is not a fraction of the total number of boards.");
    }

    int maxIndex = (NBV1720TOTAL/NB1720PERLINK)/NBLINKSPERFE - 1;
    if(feIndex < 0 || feIndex > maxIndex){
      printf("Front end index must be between 0 and %d\n", maxIndex);
      exit(1);
    }

    int firstLink = (feIndex % (NBLINKSPERA3818 / NBLINKSPERFE)) * NBLINKSPERFE;
    int lastLink = firstLink + NBLINKSPERFE - 1;
    for (int iLink=firstLink; iLink <= lastLink; iLink++)
    {
      for (int iBoard=0; iBoard < NB1720PERLINK; iBoard++)
      {

        printf("<<< Begin of Init \n feIndex:%i, link:%i, board:%i\n",
            feIndex, iLink, iBoard);

        // Compose unique module ID
        int moduleID = feIndex*NBLINKSPERFE*NB1720PERLINK + iLink*NB1720PERLINK + iBoard;

        // Create module objects
        ov1720.push_back(v1720CONET2(feIndex, iLink, iBoard, moduleID));
        ov1720.back().verbose = 1;

        // Setup ODB record (create if necessary)
        ov1720.back().SetODBRecord(hDB,seq_callback);

        // Open Optical interface
        printf("Opening optical interface Link %d, Board %d\n", iLink, iBoard);
        CAENComm_ErrorCode sCAEN;
        sCAEN = ov1720.back().Connect();
        if (sCAEN != CAENComm_Success) {
          cm_msg(MERROR, "fe", "Link#:%d iBoard#:%d error:%d", iLink, iBoard, sCAEN);
        }
        else {
          tNActivev1720++;
          printf("Link#:%d Board#:%d Module_Handle[%d]:%d (active:%d)\n",
              iLink, iBoard, moduleID, ov1720.back().GetHandle(), tNActivev1720);

          ov1720.back().InitializeForAcq();
        }
      }
    }
  }

  // Lock to one core on the processor. Need to be moved to TFeCommon
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(4, &mask);  // arbitrarily chose 4
  if( sched_setaffinity(0, sizeof(mask), &mask) < 0 ){
    printf("ERROR: affinity not set\n");
  }

  if(feIndex == -1 )
    printf(">>> End of Init. %d active v1720. Expected %d\n\n",
        tNActivev1720,NB1720PERLINK*NBLINKSPERA3818);
  else
    printf(">>> End of Init. %d active v1720. Expected %d\n\n",
        tNActivev1720,NB1720PERLINK*NBLINKSPERFE);
#endif  //SIMULATION


  set_equipment_status(equipment[0].name, "Initialized", "#00ff00");

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
INT frontend_exit(){

  set_equipment_status(equipment[0].name, "Exiting...", "#FFFF00");

  for (itv1720 = ov1720.begin(); itv1720 != ov1720.end(); ++itv1720) {
    if (itv1720->IsConnected()){
      itv1720->Disconnect();
    }
  }
  set_equipment_status(equipment[0].name, "Exited", "#00ff00");
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
INT begin_of_run(INT run_number, char *error){

  set_equipment_status(equipment[0].name, "Starting run...", "#FFFF00");

  printf("<<< Begin of begin_of_run\n");

  CAENComm_ErrorCode sCAEN = CAENComm_Success;
  //we've decided to only initialize at start of frontend
  //if odb stuff is changed, will need to restart fe to take effect.

  // Reset and Start v1720s
  for (itv1720 = ov1720.begin(); itv1720 != ov1720.end(); ++itv1720) {
    if (! itv1720->IsConnected()) continue;   // Skip unconnected board
    printf("Starting module iBoard %d\n", itv1720->GetModuleID());
    // Reset card (Alex) Moved to the beginning
    //sCAEN = ov1720[iBoard]->WriteReg(v1720_SW_RESET, 0x1);
    //sCAEN = ov1720[iBoard]->WriteReg(v1720_SW_CLEAR, 0x1);
    // Start run then wait for trigger
    itv1720->StartRun();
  }

  runInProgress = true;

  printf(">>> End of begin_of_run\n\n");
  set_equipment_status(equipment[0].name, "Started run", "#00ff00");

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
INT end_of_run(INT run_number, char *error){

  DWORD buffLvl;

  set_equipment_status(equipment[0].name, "Ending run...", "#FFFF00");

  printf("<<< Beging of end_of_run \n");
  // Stop run
  CAENComm_ErrorCode sCAEN = CAENComm_Success;

  for (itv1720 = ov1720.begin(); itv1720 != ov1720.end(); ++itv1720) {
    if (itv1720->IsConnected()) {  // Skip unconnected board
      sCAEN = itv1720->StopRun();
    }
  }

  runOver = false;
  runStopRequested = false;
  runInProgress = false;

#if SIMULATION
  sCAEN = ov1720[0].Poll(&buffLvl);
#else
  sCAEN = ov1720[0].Poll(&buffLvl);
  if(buffLvl != 0x0) {
    cm_msg(MERROR, "feov1720:EOR", "Events left in the v1720: %d",buffLvl);
  }
#endif

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
  char str[128];
  static DWORD evlimit;

  if (runStopRequested && !runOver) {
    db_set_value(hDB,0,"/logger/channels/0/Settings/Event limit", &evlimit, sizeof(evlimit), 1, TID_DWORD);
    if (cm_transition(TR_STOP, 0, str, sizeof(str), ASYNC, FALSE) != CM_SUCCESS) {
      cm_msg(MERROR, "feov1720", "cannot stop run: %s", str);
    }
    runInProgress = false;
    runOver = true;
    cm_msg(MERROR, "feov1720","feov1720 Stop requested");
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
{
  register int i;  // , mod=-1;
  register DWORD lam;
  register DWORD buffLvl;
  CAENComm_ErrorCode sCAEN;
  for (i = 0; i < count; i++) {
    //    if (gv1720Handle[board] < 0) continue;   // Skip unconnected board
    lam = 0;
    sCAEN = ov1720[0].Poll(&buffLvl);

    if (debugpolling) printf("loop: %d lam:%x, sCAEN:%d\n", i, lam, sCAEN);

    if (buffLvl > 0) {  // Why was this set to > 5? (Alex 01/03/13)

      //event available
      Nloop = i;
      Ncount = count;
#if 0
      DWORD acqStat;
      ov1720[0].ReadReg(v1720_ACQUISITION_STATUS, &acqStat);
      ov1720[0].ReadReg(v1720_EVENT_STORED ,&buffLvl);
      printf("vme %i | acq %i | buff lvl %i | count %i\n",lam, acqStat, buffLvl, Nloop);
#endif
      if (!test) return 1;
    } else {
      ss_sleep(1);
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
 * \brief   Event readout
 *
 * Event readout routine.  This is called by the polling or interrupt routines.
 * (see mfe.c).  For each module, read the event buffer into a midas data bank.
 * If ZLE data exists, create another bank for it.  Finally, create a statistical
 * bank for data throughput analysis.
 *
 * \param   [in]  pevent Pointer to event buffer
 * \param   [in]  off Caller info (unused here), see mfe.c
 *
 * \return  Size of the event
 */
INT read_trigger_event(char *pevent, INT off) {

  if (!runInProgress) return 0;

  DWORD *pdata, eStored, eSize, eSizeTot, sn, buffLvl;

  sn = SERIAL_NUMBER(pevent);

  bk_init32(pevent);
  char bankName[5];
  char statBankName[5];
  CAENComm_ErrorCode sCAEN;

  for (itv1720 = ov1720.begin(); itv1720 != ov1720.end(); ++itv1720) {

    if (! itv1720->IsConnected()) continue;   // Skip unconnected board

    // >>> Get time before read (for data throughput analysis. To be removed)
    timeval tv;
    gettimeofday(&tv,0);
    suseconds_t usStart = tv.tv_usec;

    // >>> Read a few registers
    sCAEN = itv1720->ReadReg(V1720_EVENT_STORED, &eStored);
    sCAEN = itv1720->ReadReg(V1720_EVENT_SIZE, &eSize);
    eSizeTot = eSize; // eSize will be decremented later on
    buffLvl = eStored;
    DWORD lam;
    sCAEN = itv1720->ReadReg(V1720_VME_STATUS, &lam);
    //    printf("S/N:%i B:%i eSt:%i eSze%i Lvl:%i Lam:%i \n",sn,iBoard,eStored,eSize,buffLvl,lam);

    if(debugtrigger){
      printf("Module:%02d Hndle:%d S/N:%8.8d Nloop:%d/%d (%5.2f) sCAEN:%d Event Stored:0x%x Event Size:0x%x\n"
          , itv1720->GetModuleID(), itv1720->GetHandle(), sn, Nloop, Ncount, (100.*Nloop/Ncount), sCAEN, eStored, eSize);
    }

    // >>> Calculate module number
    int module = itv1720->GetModuleID();

    // >>> create data bank
    if(itv1720->IsZLEData()){
      sprintf(bankName, "ZL%02d", module);
    }
    else{
      sprintf(bankName, "W2%02d", module);
    }
    bk_create(pevent, bankName, TID_DWORD, &pdata);

    //read event into bank
    int dwords_read = 0;
    sCAEN = itv1720->ReadEvent(pdata, &dwords_read);

    // >>> close data bank
    bk_close(pevent, pdata + dwords_read);

    //might eventually move this stuff into helper

    // >>> Fill Qt bank if ZLE data
    if(itv1720->IsZLEData()){
      itv1720->fillQtBank(pevent, pdata, module);
    }

    // >>> Statistical bank for data throughput analysis
    sprintf(statBankName, "ST%02d", module);
    bk_create(pevent, statBankName, TID_DWORD, &pdata);
    *pdata++ = module;
    *pdata++ = eStored;
    *pdata++ = eSizeTot;
    *pdata++ = buffLvl;
    gettimeofday(&tv,0);
    *pdata++ = usStart;
    *pdata++ = tv.tv_usec;
    bk_close(pevent, pdata);

  }

  //primitive progress bar
  if (sn % 100 == 0) printf(".");

  return bk_size(pevent);
}
