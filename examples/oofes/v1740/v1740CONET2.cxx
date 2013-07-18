/*****************************************************************************/
/**
\file v1740CONET2.cxx

## Contents

This file contains the class implementation for the v1740 module driver.
 *****************************************************************************/

#include "v1740CONET2.hxx"
#include <stdlib.h>

#define UNUSED(x) ((void)(x)) //!< Suppress compiler warnings

using namespace std;

//! Configuration string for this module. (ODB: /Equipment/[eq_name]/Settings/[board_name]/)
const char * v1740CONET2::config_str[] = {\
    "setup = INT : 0",\
    "Acq mode = INT : 1",\
    "Group Configuration = DWORD : 0",\
    "Buffer organization = INT : 8",\
    "Custom size = INT : 0",\
    "Group Mask = DWORD : 0xFF",\
    "Trigger Source = DWORD : 0x40000000",\
    "Trigger Output = DWORD : 0x40000000",\
    "Post Trigger = DWORD : 200",\
    "Threshold = DWORD[8] :",\
    "[0] 2080",\
    "[1] 1900",\
    "[2] 2080",\
    "[3] 9",\
    "[4] 9",\
    "[5] 9",\
    "[6] 9",\
    "[7] 9",\
    "DAC = DWORD[8] :",\
    "[0] 35000",\
    "[1] 35000",\
    "[2] 35000",\
    "[3] 35000",\
    "[4] 35000",\
    "[5] 35000",\
    "[6] 9190",\
    "[7] 9190",\
    NULL
};

/**
 * \brief   Constructor for the module object
 *
 * Set the basic hardware parameters
 *
 * \param   [in]  link      Optical link number
 * \param   [in]  board     Board number on the optical link
 * \param   [in]  moduleID  Unique ID assigned to module
 */
v1740CONET2::v1740CONET2(int link, int board, int moduleID) :
        _link(link), _board(board), _moduleID(moduleID)
{
  _handle = -1;
  _odb_handle = 0;
  verbose = 0;
  _settings_loaded=false;
  _settings_touched=false;
  _running = false;
}
/**
 * \brief   Destructor for the module object
 *
 * Nothing to do.
 */
v1740CONET2::~v1740CONET2() {}

/**
 * \brief   Get short string identifying the module's board number
 *
 * \return  name string
 */
string v1740CONET2::GetName()
{
  stringstream txt;
  txt << "B" << _board;
  return txt.str();
}
/**
 * \brief   Get connected status
 *
 * \return  true if board is connected
 */
bool v1740CONET2::IsConnected()
{
  return (_handle >= 0);
}
/**
 * \brief   Get run status
 *
 * \return  true if run is started
 */
bool v1740CONET2::IsRunning()
{
  return _running;
}
/**
 * \brief   Connect the board through the optical link
 *
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::Connect()
{
  if (verbose) cout << "Opening device (l,b) = (" << _link << "," << _board << ")" << endl;
  CAENComm_ErrorCode sCAEN;

#if SIMULATION  //Simulate success opening device
  sCAEN = CAENComm_Success;
  _handle = 1;  //random
#else
  sCAEN = CAENComm_OpenDevice(CAENComm_PCIE_OpticalLink, _link, _board, 0, &_handle);
#endif

  if (sCAEN != CAENComm_Success) _handle = -1;
  return sCAEN;
}
/**
 * \brief   Disconnect the board through the optical link
 *
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::Disconnect()
{
  if (verbose) cout << "Closing device (l,b) = (" << _link << "," << _board << ")" << endl;
  CAENComm_ErrorCode sCAEN;

#if SIMULATION  //Simulate success closing device
  sCAEN = CAENComm_Success;
#else
  sCAEN = CAENComm_CloseDevice(_handle);
#endif


  _handle = -1;
  return sCAEN;
}
/**
 * \brief   Start data acquisition
 *
 * Write to Acquisition Control reg to put board in RUN mode. If ODB
 * settings have been changed, re-initialize the board with the new settings.
 * Set _running flag true.
 *
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::StartRun()
{
  if (verbose) cout << GetName() << "::StartRun()\n";
  if (IsRunning()) {
    cout << "Error: trying to start already started board" << endl;
    return (CAENComm_ErrorCode)1;
  }
  if (!IsConnected()) {
    cout << "Error: trying to start disconnected board" << endl;
    return (CAENComm_ErrorCode)1;
  }
  if (_settings_touched)
  {
    cm_msg(MINFO, "feoV1740", "Note: settings on board %s touched. Re-initializing board.",
        GetName().c_str());
    cout << "reinitializing" << endl;
    InitializeForAcq();
  }

  CAENComm_ErrorCode e = ov1740_AcqCtl(_handle, V1740_RUN_START);
  if (e == CAENComm_Success) _running=true;
  return e;
}
/**
 * \brief   Start data acquisition
 *
 * Write to Acquisition Control reg to put board in STOP mode.
 * Set _running flag false.
 *
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::StopRun()
{
  CAENComm_ErrorCode e = ov1740_AcqCtl(_handle, V1740_RUN_STOP);
  if (e == CAENComm_Success) _running=false;
  return e;
}
/**
 * \brief   Setup board registers using preset (see ov1720.c:ov1720_Setup())
 *
 * Setup board registers using a preset defined in the midas file ov1720.c
 * - Mode 0x0: "Setup Skip\n"
 * - Mode 0x1: "Trigger from FP, 8ch, 1Ks, postTrigger 800\n"
 * - Mode 0x2: "Trigger from LEMO\n"
 *
 * \param   [in]  mode Configuration mode number
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::SetupPreset(int mode)
{
#if SIMULATION
  return CAENComm_Success;
#else
  return ov1740_Setup(_handle, mode);
#endif
}
/**
 * \brief   Control data acquisition
 *
 * Write to Acquisition Control reg
 *
 * \param   [in]  operation acquisition mode (see v1720.h)
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::AcqCtl(uint32_t operation)
{
#if SIMULATION
  return CAENComm_Success;
#else
  return ov1740_AcqCtl(_handle, operation);
#endif
}
/**
 * \brief   Read 32-bit register
 *
 * \param   [in]  address  address of the register to read
 * \param   [out] val      value read from register
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::ReadReg(DWORD address, DWORD *val)
{
  if (verbose >= 2) cout << GetName() << "::ReadReg(" << hex << address << ")" << endl;
#if SIMULATION
  return CAENComm_Success;
#else
  return CAENComm_Read32(_handle, address, val);
#endif
}
/**
 * \brief   Write to 32-bit register
 *
 * \param   [in]  address  address of the register to write to
 * \param   [in]  val      value to write to the register
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::WriteReg(DWORD address, DWORD val)
{
  if (verbose >= 2) cout << GetName() << "::WriteReg(" << hex << address << "," << val << ")" << endl;
#if SIMULATION
  return CAENComm_Success;
#else
  return CAENComm_Write32(_handle, address, val);
#endif
}

//! Maximum size of data to read using BLT (32-bit) cycle
#define MAX_BLT_READ_SIZE 10000
/**
 * \brief   Read event buffer
 *
 * Read the event buffer for this module using BLT (32-bit) cycles.
 * This function reads nothing if EVENT_SIZE register was zero.
 *
 * \param   [out]  data         Where to write content of event buffer
 * \param   [out]  dwords_read  Number of DWORDs read from the buffer
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::ReadEvent(DWORD *data, int *dwords_read)
{
  DWORD size_remaining, *data_pos = data, to_read;
  int tempnw = 0;
  *dwords_read = 0;

#if SIMULATION  //read maximum size allowed
  CAENComm_ErrorCode sCAEN = CAENComm_Success;
  size_remaining = MAX_BLT_READ_SIZE;
#else
  CAENComm_ErrorCode sCAEN = ReadReg(V1740_EVENT_SIZE, &size_remaining);
#endif

  if (verbose>=2) cout << "Start of ReadEvent... "
      << "size_rem = " << size_remaining << " " << sCAEN << endl;

  while (size_remaining > 0 && sCAEN == CAENComm_Success)
  {
#if SIMULATION  //read random 32-bit integers
    to_read = size_remaining > MAX_BLT_READ_SIZE ? MAX_BLT_READ_SIZE : size_remaining;
    for (WORD i = 0; i < to_read; i++)
      *data_pos++ = rand();
    sCAEN = CAENComm_Success;
    tempnw = to_read; //simulate success reading all data
#else
    to_read = size_remaining > MAX_BLT_READ_SIZE ? MAX_BLT_READ_SIZE : size_remaining;
    sCAEN = CAENComm_BLTRead(_handle, V1740_EVENT_READOUT_BUFFER, data_pos, to_read, &tempnw);
#endif

    if (verbose>=2) cout << sCAEN << " = BLTRead(handle=" << _handle
        << ", addr=" << V1740_EVENT_READOUT_BUFFER
        << ", data_pos=" << data_pos
        << ", to_read=" << to_read
        << ", tempnw returned " << tempnw << ");" << endl;
    *dwords_read += tempnw;
    size_remaining -= tempnw;
    data_pos += tempnw;
  }
  if (verbose>=2) cout << "End of ReadEvent... "
      << "dwords_read = " << *dwords_read << endl;;

  return sCAEN;
}
/**
 * \brief   Send a software trigger to the board
 *
 * Send a software trigger to the board.  May require
 * software triggers to be enabled in register.
 *
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1740CONET2::SendTrigger()
{
  if (verbose) cout << "Sending Trigger (l,b) = (" << _link << "," << _board << ")" << endl;
  return WriteReg(V1740_SOFTWARE_TRIGGER, 0x1);
}
/**
 * \brief   Set the ODB record for this module
 *
 * Create a record for the board with settings from the configuration
 * string (v1740CONET2::config_str) if it doesn't exist or merge with
 * existing record. Create hotlink with callback function for when the
 * record is updated.  Get the handle to the record.
 *
 * Ex: For a frontend with index number 2 and board number 0, this
 * record will be created/merged:
 *
 * /Equipment/FEv1740I2/Settings/Board0
 *
 * \param   [in]  h        main ODB handle
 * \param   [in]  cb_func  Callback function to call when record is updated
 * \return  ODB Error Code (see midas.h)
 */
int v1740CONET2::SetODBRecord(HNDLE h, void(*cb_func)(INT,INT,void*))
{
  char set_str[80];
  int status,size;

#if SIMULATION
  sprintf(set_str, "/Equipment/FEv1740_SIM/Settings/Board%d", _board);
#else
  sprintf(set_str, "/Equipment/FEv1740/Settings/Board%d", _board);
#endif

  if (verbose) cout << GetName() << "::SetODBRecord(" << h << "," << set_str << ",...)" << endl;

  //create record if doesn't exist and find key
  status = db_create_record(h, 0, set_str, strcomb(config_str));
  status = db_find_key(h, 0, set_str, &_odb_handle);
  if (status != DB_SUCCESS) cm_msg(MINFO,"FE","Key %s not found", set_str);

  //hotlink
  //  size = sizeof(config_type);
  size = sizeof(V1740_CONFIG_SETTINGS);
  status = db_open_record(h, _odb_handle, &config, size, MODE_READ, cb_func, NULL);

  //get actual record
  status = db_get_record(h, _odb_handle, &config, &size, 0);

  if (status == DB_SUCCESS) _settings_loaded = true;
  _settings_touched = true;

  return status; //== DB_SUCCESS for success
}
/**
 * \brief   Initialize the hardware for data acquisition
 *
 * Set the registers using a preset if setup != 0 in the config string, or set
 * them manually otherwise.
 *
 * \return  0 on success, -1 on error
 */
int v1740CONET2::InitializeForAcq()
{
  if (!_settings_loaded) {
    cout << "Error: cannot call InitializeForAcq() without settings loaded properly" << endl;
    return -1;    }
  if (!IsConnected())     {
    cout << "Error: trying to call InitializeForAcq() to unconnected board" << endl;
    return -1;    }
  if (IsRunning())    {
    cout << "Error: trying to call InitializeForAcq() to already running board" << endl;
    return -1;    }

  if (_settings_loaded && !_settings_touched) return 0; //don't do anything if settings haven't been changed

  CAENComm_ErrorCode sCAEN;
  uint32_t temp;

  //use preset setting if enabled
  if (config.setup != 0) {
    SetupPreset(config.setup);
    //else use odb values
  } else {

    AcqCtl(config.acq_mode);
    printf("group config 0x%x\n", config.group_config);
    WriteReg(V1740_GROUP_CONFIG, config.group_config);
    WriteReg(V1740_GROUP_CONFIG        , config.group_config);
    WriteReg(V1740_BUFFER_ORGANIZATION   , config.buffer_organization);
    WriteReg(V1740_CUSTOM_SIZE       , config.custom_size);
    WriteReg(V1740_GROUP_EN_MASK       , config.group_mask);
    AcqCtl(V1740_COUNT_ACCEPTED_TRIGGER);
    WriteReg(V1740_TRIG_SRCE_EN_MASK     , config.trigger_source);
    WriteReg(V1740_FP_TRIGGER_OUT_EN_MASK, config.trigger_output);
    WriteReg(V1740_POST_TRIGGER_SETTING  , config.post_trigger);
    ReadReg(V1740_GROUP_CONFIG, &temp);
    WriteReg(V1740_MONITOR_MODE, 0x3);  // buffer occupancy
    printf("group config 0x%x\n",  temp);
    /*sCAEN = ov1740_GroupConfig(GetHandle()
      , config.trigger_positive_pulse == 1 ? V1740_TRIGGER_OVERTH : V1740_TRIGGER_UNDERTH);*/
    WriteReg(V1740_VME_CONTROL, V1740_ALIGN64);

    // Set individual channel stuff
    for (int i=0;i<8;i++) {
      //group threshold
      sCAEN = ov1740_GroupSet(GetHandle(), i, V1740_GROUP_THRESHOLD, config.threshold[i]);
      sCAEN = ov1740_GroupGet(GetHandle(), i, V1740_GROUP_THRESHOLD, &temp);
      printf("Threshold[%i] = %d \n", i, temp);
      //set DAC
      sCAEN = ov1740_GroupDACSet(GetHandle(), i, config.dac[i]);
      printf("DAC[%i] = %d \n", i, temp);
      //disable channel triggers
      WriteReg(V1740_GROUP_CH_TRG_MASK + (i<<8), 0);
    }

    // Doesn't set the ID in the data stream!
    WriteReg( V1740_BOARD_ID, 6);
    ov1740_Status(GetHandle());
  }

  _settings_touched = false;
  UNUSED(sCAEN);

  //ready to do startrun

  return 0;
}
/**
 * \brief Check board type
 *
 * Check if board type is v1740, if not issue a warning
 */
void v1740CONET2::CheckBoardType()
{
  // Verify Board Type
  uint32_t version = 0;
  const uint32_t v1740_board_type = 0x04;
  CAENComm_ErrorCode sCAEN = ReadReg(V1740_BOARD_INFO, &version);
  if((version & 0xFF) != v1740_board_type)
    cm_msg(MINFO,"feoV1740","*** WARNING *** Trying to use a v1740 frontend with another"
        " type of board.  Results will be unexpected!");

  UNUSED(sCAEN);
}
/**
 * \brief Misc reg printout, for debug purposes
 */
void v1740CONET2::ShowRegs()
{
#if !SIMULATION
  DWORD temp;
  CAENComm_ErrorCode sCAEN;
  cout << "   *** regs: ";
  sCAEN = CAENComm_Read32(_handle, V1740_EVENT_SIZE, &temp); cout << "E_SIZE=0x" << hex <<  temp << " ";
  sCAEN = CAENComm_Read32(_handle, V1740_EVENT_STORED, &temp); cout << "E_STORED=0x" << hex <<  temp << " ";
  sCAEN = CAENComm_Read32(_handle, V1740_GROUP_BUFFER_OCCUPANCY, &temp); cout << "GRP_BUFF_OCC[0]=0x" << temp << " ";
  sCAEN = CAENComm_Read32(_handle, V1740_ACQUISITION_STATUS, &temp); cout << "ACQ_STATUS=0x" << temp << " lam=" << (temp & 0x4);
  //sCAEN = CAENComm_Read32(_handle, V1740_BLT_EVENT_NB, &temp); cout << "BLT_EVENT_NB=0x" << temp << " ";
  //sCAEN = CAENComm_Read32(_handle, V1740_GROUP_CONFIG, &temp); cout << "GROUP_CONFIG= " << temp << " ";
  //sCAEN = CAENComm_Read32(_handle, V1740_GROUP_EN_MASK, &temp); cout << "GROUP_EN_MASK= " << temp << " ";
  cout << dec << endl;
  UNUSED(sCAEN);
#endif
}
