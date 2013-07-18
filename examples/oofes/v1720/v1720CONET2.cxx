/*****************************************************************************/
/**
\file v1720CONET2.cxx

## Contents

This file contains the class implementation for the v1720 module driver.
 *****************************************************************************/

#include "v1720CONET2.hxx"
#include <stdlib.h>

#define UNUSED(x) ((void)(x)) //!< Suppress compiler warnings

using namespace std;

//! Configuration string for this module. (ODB: /Equipment/[eq_name]/Settings/[board_name]/)
const char * v1720CONET2::config_str[] = {\
    "setup = INT : 0",\
    "Acq mode = INT : 3",\
    "Channel Configuration = DWORD : 131088",\
    "Buffer organization = INT : 10",\
    "Custom size = INT : 625",\
    "Channel Mask = DWORD : 255",\
    "Trigger Source = DWORD : 1073741824",\
    "Trigger Output = DWORD : 1073741824",\
    "Post Trigger = DWORD : 1000",\
    /*"fp_io_ctrl   = DWORD : 0x104", */ \
    "almost_full  = DWORD : 850",\
    /*"fp_lvds_io_ctrl = DWORD : 0x22", */  \
    /*"Trigger Positive Pulse = BOOL : n",*/ \
    /*"Pre-Record_ZS = WORD : 4",     */ \
    /*"Post-Record_ZS = WORD : 4",      */ \
    "Threshold = DWORD[8] :",\
    "[0] 2200",\
    "[1] 2108",\
    "[2] 2100",\
    "[3] 2108",\
    "[4] 9",\
    "[5] 9",\
    "[6] 9",\
    "[7] 9",\
    "NbOUThreshold = DWORD[8] :",\
    "[0] 2",\
    "[1] 2",\
    "[2] 2",\
    "[3] 2",\
    "[4] 2",\
    "[5] 2",\
    "[6] 2",\
    "[7] 2",\
    "ZS_Threshold = INT[8] :",\
    "[0] 0x80000d93",\
    "[1] 0x80000d93",\
    "[2] 0x80000d93",\
    "[3] 0x80000d93",\
    "[4] 0x80000d93",\
    "[5] 0x80000d93",\
    "[6] 0x80000d93",\
    "[7] 0x80000d93",\
    "ZS_NsAmp = DWORD[8] :",\
    "[0] 0x50005",\
    "[1] 0x50005",\
    "[2] 0x50005",\
    "[3] 0x50005",\
    "[4] 0x50005",\
    "[5] 0x50005",\
    "[6] 0x50005",\
    "[7] 0x50005",\
    "DAC = DWORD[8] :",\
    "[0] 10000",\
    "[1] 10000",\
    "[2] 10000",\
    "[3] 10000",\
    "[4] 10000",\
    "[5] 10000",\
    "[6] 10000",\
    "[7] 10000",\
    NULL };

#if SIMULATION
/**
 * \brief   Constructor for the module object
 *
 * Set the basic hardware parameters
 *
 * \param   [in]  board     Board number on the optical link
 */
v1720CONET2::v1720CONET2(int board) : _board(board)
{
  _handle = -1;
  _odb_handle = 0;
  verbose = 0;
  _running=false;
  _settings_loaded=false;
  _settings_touched=false;
  mZLE=false;
  mDataType = 0;
}
#else
/**
 * \brief   Constructor for the module object
 *
 * Set the basic hardware parameters
 *
 * \param   [in]  feindex   Frontend index number
 * \param   [in]  link      Optical link number
 * \param   [in]  board     Board number on the optical link
 * \param   [in]  moduleID  Unique ID assigned to module
 */
v1720CONET2::v1720CONET2(int feindex, int link, int board, int moduleID)
: _feindex(feindex), _link(link), _board(board), _moduleID(moduleID)
{
  _handle = -1;
  _odb_handle = 0;
  verbose = 0;
  _settings_loaded=false;
  _settings_touched=false;
  _running=false;
  mZLE=false;
  mDataType = 0;
}
#endif  //SIMULATION
/**
 * \brief   Destructor for the module object
 *
 * Nothing to do.
 */
v1720CONET2::~v1720CONET2()
{
}

/**
 * \brief   Get short string identifying the module's index, link and board number
 *
 * \return  name string
 */
string v1720CONET2::GetName()
{
  stringstream txt;
#if SIMULATION
  txt << "B" << _board;
#else
  txt << "F" << _feindex << _link << _board;
#endif
  return txt.str();
}
/**
 * \brief   Get connected status
 *
 * \return  true if board is connected
 */
bool v1720CONET2::IsConnected()
{
  return (_handle >= 0);
}
/**
 * \brief   Get run status
 *
 * \return  true if run is started
 */
bool v1720CONET2::IsRunning()
{
  return _running;
}
/**
 * \brief   Connect the board through the optical link
 *
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1720CONET2::Connect()
{
  if (verbose) cout << GetName() << "::Connect()\n";
  if (IsConnected()) { cout << "Error: trying to connect already connected board" << endl;
  return (CAENComm_ErrorCode) 1; }
#if SIMULATION
  if (verbose) cout << "Opening device " << _board << endl;
#else
  if (verbose) cout << "Opening device (i,l,b) = (" << _feindex << "," << _link << "," << _board << ")" << endl;
#endif

  CAENComm_ErrorCode sCAEN;

#if SIMULATION
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
CAENComm_ErrorCode v1720CONET2::Disconnect()
{
  if (verbose) cout << GetName() << "::Disconnect()\n";
  if (!IsConnected()) { cout << "Error: trying to disconnect already disconnected board" << endl;  return (CAENComm_ErrorCode)1; }
  if (IsRunning()) { cout << "Error: trying to disconnect running board" << endl;  return (CAENComm_ErrorCode)1; }
#if SIMULATION
  if (verbose) cout << "Closing device " << _board << endl;
#else
  if (verbose) cout << "Closing device (i,l,b) = (" << _feindex << "," << _link << "," << _board << ")" << endl;
#endif

  CAENComm_ErrorCode sCAEN;

#if SIMULATION
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
CAENComm_ErrorCode v1720CONET2::StartRun()
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
    cm_msg(MINFO, "feoV1720", "Note: settings on board %s touched. Re-initializing board.",
        GetName().c_str());
    cout << "reinitializing" << endl;
    InitializeForAcq();
  }

  CAENComm_ErrorCode e = AcqCtl(V1720_RUN_START);
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
CAENComm_ErrorCode v1720CONET2::StopRun()
{
  if (verbose) cout << GetName() << "::StopRun()\n";
  if (!IsRunning()) { cout << "Error: trying to stop already stopped board" << endl; return (CAENComm_ErrorCode)1; }
  if (!IsConnected()) { cout << "Error: trying to stop disconnected board" << endl; return (CAENComm_ErrorCode)1; }
  CAENComm_ErrorCode e = AcqCtl(V1720_RUN_STOP);
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
CAENComm_ErrorCode v1720CONET2::SetupPreset(int mode)
{
#if SIMULATION
  return CAENComm_Success;
#else
  return ov1720_Setup(_handle, mode);
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
CAENComm_ErrorCode v1720CONET2::AcqCtl(uint32_t operation)
{
#if SIMULATION
  return CAENComm_Success;
#else
  return ov1720_AcqCtl(_handle, operation);
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
CAENComm_ErrorCode v1720CONET2::ChannelConfig(uint32_t operation)
{
#if SIMULATION
  return CAENComm_Success;
#else
  return ov1720_ChannelConfig(_handle, operation);
#endif
}
/**
 * \brief   Read 32-bit register
 *
 * \param   [in]  address  address of the register to read
 * \param   [out] val      value read from register
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1720CONET2::ReadReg(DWORD address, DWORD *val)
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
CAENComm_ErrorCode v1720CONET2::WriteReg(DWORD address, DWORD val)
{
  if (verbose >= 2) cout << GetName() << "::WriteReg(" << hex << address << "," << val << ")" << endl;
#if SIMULATION
  return CAENComm_Success;
#else
  return CAENComm_Write32(_handle, address, val);
#endif
}
/**
 * \brief   Poll Event Stored register
 *
 * Check Event Stored register for any event stored
 *
 * \param   [out]  val     Number of events stored
 * \return  CAENComm Error Code (see CAENComm.h)
 */
CAENComm_ErrorCode v1720CONET2::Poll(DWORD *val)
{
#if SIMULATION
  return CAENComm_Success;
#else
  return CAENComm_Read32(_handle, V1720_EVENT_STORED, val);
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
CAENComm_ErrorCode v1720CONET2::ReadEvent(DWORD *data, int *dwords_read)
{
  if (!IsConnected()) {
    cout << "Error: trying to ReadEvent disconnected board" << endl;
    return (CAENComm_ErrorCode)1;
  }
  DWORD size_remaining, *data_pos = data, to_read;
  int tempnw = 0;
  *dwords_read = 0;

  //read size of event to be read
#if SIMULATION
  CAENComm_ErrorCode sCAEN = CAENComm_Success;
  size_remaining = MAX_BLT_READ_SIZE;
#else
  CAENComm_ErrorCode sCAEN = ReadReg(V1720_EVENT_SIZE, &size_remaining);
#endif

  while (size_remaining > 0 && sCAEN == CAENComm_Success)
  {
#if SIMULATION
    to_read = size_remaining > MAX_BLT_READ_SIZE ? MAX_BLT_READ_SIZE : size_remaining;
    for (WORD i = 0; i < to_read; i++)
      *data_pos++ = rand();
    sCAEN = CAENComm_Success;
    tempnw = to_read; //simulate success reading all data
#else
    //calculate amount of data to be read in this iteration
    to_read = size_remaining > MAX_BLT_READ_SIZE ? MAX_BLT_READ_SIZE : size_remaining;
    sCAEN = CAENComm_BLTRead(_handle, V1720_EVENT_READOUT_BUFFER, data_pos, to_read, &tempnw);
#endif

    if (verbose >= 2) cout << sCAEN << " = BLTRead(handle=" << _handle
        << ", addr=" << V1720_EVENT_READOUT_BUFFER
        << ", data_pos=" << data_pos
        << ", to_read=" << to_read
        << ", tempnw returned " << tempnw << ");" << endl;

    //increment pointers/counters
    *dwords_read += tempnw;
    size_remaining -= tempnw;
    data_pos += tempnw;
  }
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
CAENComm_ErrorCode v1720CONET2::SendTrigger()
{
  if (verbose) cout << GetName() << "::SendTrigger()" << endl;
  if (!IsConnected()) { cout << "Error: trying to SendTrigger disconnected board" << endl; return (CAENComm_ErrorCode)1; }
#if SIMULATION
  if (verbose) cout << "Sending Trigger " << _board << endl;
#else
  if (verbose) cout << "Sending Trigger (l,b) = (" << _link << "," << _board << ")" << endl;
#endif

  return WriteReg(V1720_SW_TRIGGER, 0x1);
}
/**
 * \brief   Set the ODB record for this module
 *
 * Create a record for the board with settings from the configuration
 * string (v1720CONET2::config_str) if it doesn't exist or merge with
 * existing record. Create hotlink with callback function for when the
 * record is updated.  Get the handle to the record.
 *
 * Ex: For a frontend with index number 2 and board number 0, this
 * record will be created/merged:
 *
 * /Equipment/FEv1720I2/Settings/Board0
 *
 * \param   [in]  h        main ODB handle
 * \param   [in]  cb_func  Callback function to call when record is updated
 * \return  ODB Error Code (see midas.h)
 */
int v1720CONET2::SetODBRecord(HNDLE h, void(*cb_func)(INT,INT,void*))
{
  char set_str[200];

#if SIMULATION
  sprintf(set_str, "/Equipment/FEv1720_SIM/Settings/Board%d", _board);
#else
  if(_feindex == -1)
    sprintf(set_str, "/Equipment/FEv1720I/Settings/Board%d", _moduleID);
  else
    sprintf(set_str, "/Equipment/FEv1720I%0d/Settings/Board%d", _feindex, _moduleID);
#endif

  if (verbose) cout << GetName() << "::SetODBRecord(" << h << "," << set_str << ",...)" << endl;
  int status,size;
  //create record if doesn't exist and find key
  status = db_create_record(h, 0, set_str, strcomb(config_str));
  status = db_find_key(h, 0, set_str, &_odb_handle);
  if (status != DB_SUCCESS) cm_msg(MINFO,"FE","Key %s not found", set_str);

  //hotlink
  size = sizeof(V1720_CONFIG_SETTINGS);
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
 * ### Initial setup:
 * - Set FP I/O Ctrl (0x811C) to default settings (output trigger).
 * - Do software reset + clear.
 * - Set up busy daisy chaining
 * - Put acquisition on stop.
 *
 * ### Checks
 * - AMC firmware version (and check that each channel has the same AMC firmware)
 * - ROC firmware version
 * - board type
 *
 * ### Set registers
 * Use a preset if setup != 0 in the config string, or set them manually otherwise.
 *
 * \return  0 on success, -1 on error
 */
int v1720CONET2::InitializeForAcq()
{
  if (verbose) cout << GetName() << "::InitializeForAcq()" << endl;

  if (!_settings_loaded) {
    cout << "Error: cannot call InitializeForAcq() without settings loaded properly" << endl;
    return -1;    }
  if (!IsConnected())     {
    cout << "Error: trying to call InitializeForAcq() to unconnected board" << endl;
    return -1;    }
  if (IsRunning())    {
    cout << "Error: trying to call InitializeForAcq() to already running board" << endl;
    return -1;    }

  //don't do anything if settings haven't been changed
  if (_settings_loaded && !_settings_touched) return 0;

  CAENComm_ErrorCode sCAEN;

  // Set register V1720_FP_IO_CONTROL (0x811C) to default settings
  // (output trigger) will set the board that output the clock latter
  sCAEN = WriteReg(V1720_FP_IO_CONTROL, 0x00000000);

  // Clear the board
  sCAEN = WriteReg(V1720_SW_RESET, 0x1);
  sCAEN = WriteReg(V1720_SW_CLEAR, 0x1);

  // Setup Busy daisy chaining
  printf("\nBusy daisy chaining \n\n");
  sCAEN = WriteReg(V1720_FP_IO_CONTROL,           0x104);
  //sCAEN = WriteReg(V1720_FP_LVDS_IO_CRTL,         0x22);
  sCAEN = WriteReg(V1720_ACQUISITION_CONTROL,         0x100);

#if SIMULATION
  cm_msg(MINFO,"feoV1720","Simulation, no firmware check");
#else
  // Firmware version check
  // read each AMC firmware version
  // [31:16] Revision date Y/M/DD
  // [15:8] Firmware Revision (X)
  // [7:0] Firmware Revision (Y)
  // eg 0x760C0103 is 12th June 07, revision 1.3
  int addr = 0;
  uint32_t version = 0;
  uint32_t prev_chan = 0;
  // Hardcode correct firmware verisons
  // Current release 3.4_0.11 Feb 2012
  const uint32_t amc_fw_ver = 0xc102000b;
  const uint32_t roc_fw_ver = 0xc2080304;
  const uint32_t roc_fw_ver_test = 0xc5250306;   //new version we're testing

  for(int iCh=0;iCh<8;iCh++) {
    addr = 0x108c | (iCh << 8);
    sCAEN = ReadReg(addr, &version);
    if((iCh != 0) && (prev_chan != version)) {
      cm_msg(MERROR, "feoV1720","Error AMC Channels have different Firmware \n");
    }
    prev_chan = version;
  }
  cm_msg(MINFO,"feoV1720","Format: YMDD:XX.YY");
  if(version != amc_fw_ver) {
    cm_msg(MERROR,"feoV1720","Incorrect AMC Firmware Version: 0x%08x", version);
  }
  else {
    cm_msg(MINFO,"feoV1720","AMC Firmware Version: 0x%08x", version);
  }

  // read ROC firmware revision
  // Format as above
  sCAEN = ReadReg(V1720_ROC_FPGA_FW_REV, &version);
  switch (version)
  {
  case roc_fw_ver:
    cm_msg(MINFO,"feoV1720","ROC Firmware Version: 0x%08x", version);
    break;
  case roc_fw_ver_test:
    cm_msg(MINFO,"feoV1720","*** WARNING *** using new ROC Firmware Version: 0x%08x", version);
    break;
  default:
    cm_msg(MERROR,"feoV1720","Incorrect ROC Firmware Version: 0x%08x", version);
    break;
  }

  // Verify Board Type
  const uint32_t v1720_board_type = 0x03;
  sCAEN = ReadReg(V1720_BOARD_INFO, &version);
  if((version & 0xFF) != v1720_board_type)
    cm_msg(MINFO,"feoV1720","*** WARNING *** Trying to use a v1720 frontend with another"
        " type of board.  Results will be unexpected!");

#endif  //SIMULATION

  this->getChannelConfig(config.channel_config);

  //use preset setting if enabled
  if (config.setup != 0) SetupPreset(config.setup);
  //else use odb values
  else
  {
    //already reset/clear earlier this function, so skip here
    AcqCtl(config.acq_mode);
    WriteReg(V1720_CHANNEL_CONFIG,          config.channel_config);
    WriteReg(V1720_BUFFER_ORGANIZATION,     config.buffer_organization);
    WriteReg(V1720_CUSTOM_SIZE,             config.custom_size);
    WriteReg(V1720_CHANNEL_EN_MASK,         config.channel_mask);
    AcqCtl(V1720_COUNT_ACCEPTED_TRIGGER);
    WriteReg(V1720_TRIG_SRCE_EN_MASK,       config.trigger_source);
    WriteReg(V1720_FP_TRIGGER_OUT_EN_MASK,  config.trigger_output);
    WriteReg(V1720_POST_TRIGGER_SETTING,    config.post_trigger);
    //WriteReg(V1720_ALMOST_FULL_LEVEL,       850);
    WriteReg(V1720_MONITOR_MODE,            0x3);
    WriteReg(V1720_BLT_EVENT_NB,            0x1);
    WriteReg(V1720_VME_CONTROL,             V1720_ALIGN64);

    //set specfic channel values
    for (int iChan=0; iChan<8; iChan++)
    {
      WriteReg(V1720_CHANNEL_THRESHOLD   + (iChan<<8), config.threshold     [iChan]);
      WriteReg(V1720_CHANNEL_OUTHRESHOLD + (iChan<<8), config.nbouthreshold [iChan]);
      WriteReg(V1720_ZS_THRESHOLD        + (iChan<<8), config.zs_threshold  [iChan]);
      WriteReg(V1720_ZS_NSAMP            + (iChan<<8), config.zs_nsamp      [iChan]);
      WriteReg(V1720_CHANNEL_DAC         + (iChan<<8), config.dac           [iChan]);
      //NB: in original frontend, zst and zsn regs were set via a short calculation in the
      //frontend, not the exact values as in odb. it was not clear that this was done
      //without looking at source code, so i have changed it to just take values direct
      //from ODB.
      //On my todo list is to figure out a better way of setting up ODB settings
      //for the frontends.

    }
    //could do Status here
  }

  //todo: some kind of check to make sure stuff is set correctly???

  _settings_touched = false;
  UNUSED(sCAEN);

  //ready to do startrun

  return 0;
}
/**
 * \brief   Get data type and ZLE configuration
 *
 * Takes the channel configuration (0x8000) as parameter and checks
 * against the fields for data type (pack 2 or pack 2.5) and for ZLE
 * (Zero-length encoding).  Puts the results in fields mDataType and
 * mZLE.
 *
 * \param   [in]  aChannelConfig  Channel configuration (32-bit)
 */
void v1720CONET2::getChannelConfig(DWORD aChannelConfig){

  // Set Device, data type and packing for QT calculation later
  int dataType = ((aChannelConfig >> 11) & 0x1);
  if(((aChannelConfig >> 16) & 0xF) == 0) {
    if(dataType == 1) {
      // 2.5 pack, full data
      mDataType = 1;
      mZLE=0;
      cm_msg(MINFO,"FE","Data Type: Full data with 2.5 Packing");
    }
    else{
      // 2 pack, full data
      mZLE=0;
      mDataType = 0;
      cm_msg(MINFO,"FE","Data Type: Full data with 2 Packing");
    }
  }
  else if(((aChannelConfig >> 16) & 0xF) == 2) {
    if(dataType == 1) {
      // 2.5 pack, ZLE data
      mDataType = 3;
      cm_msg(MINFO,"FE","Data Type: ZLE data with 2.5 Packing");
      mZLE=1;
    }
    else{
      // 2 pack, ZLE data
      mDataType = 2;
      cm_msg(MINFO,"FE","Data Type: ZLE data with 2 Packing");
      mZLE=1;
    }
  }
  else{
    cm_msg(MERROR,"FE","V1720 Data format Unrecognised reg: 0x%04x", 0x8000);
  }
}
/**
 * \brief   Get ZLE setting
 *
 * Get the current ZLE setting from the channel configuration.
 *
 * \return  true if data is ZLE
 */
BOOL v1720CONET2::IsZLEData(){
  return mZLE;
}
/**
 * \brief   Fill Qt Bank
 *
 * \param   [in]  pevent  pointer to event buffer
 * \param   [in]  pZLEData  pointer to the data area of the bank
 * \param   [in]  moduleID  unique module/board ID
 */
void v1720CONET2::fillQtBank(char * pevent, uint32_t * pZLEData, int moduleID){
  // >>> Create bank.
  // QtData is the pointer to the memory location hodling the Qt data
  // Content
  // V1720 event counter
  // V1720 Trigger time tag
  // Number of Qt
  // QT format: channel 28 time 16 charge 0
  char tBankName[5];
  sprintf(tBankName,"QT%02d",moduleID);
  uint32_t *QtData;
  bk_create(pevent, tBankName, TID_DWORD, &QtData);


  // >>> copy some header words
  *QtData = *(pZLEData+2); // event counter QtData[0]
  QtData++;
  *QtData = *(pZLEData+3); // trigger time tag QtData[1]
  QtData++;

  // >>> Figure out channel mapping
  //if(mNCh==0){
  int mNCh=0;
  uint32_t mChMap[8];
  uint32_t chMask = pZLEData[1] & 0xFF;
  for(int iCh=0; iCh<8; iCh++){
    if(chMask & (1<<iCh)){
      mChMap[mNCh] = iCh;
      mNCh++;
    }
  }
  if(mNCh==0){
    // printf("No channels found for module %i! Something wrong with channel mask\n", aModule);
  }
  //}

  // >>> Skip location QtData[2]. Will be used for number of QT;
  uint32_t* nQt = QtData;
  *(nQt) = 0;
  QtData++;
  //std::cout << QtData[0] <<  " " << *(QtData-2) << " " << QtData[1] << " " << *nQt << " " << *(QtData-1)<< " ";
  // >>> Loop over ZLE data and fill up Qt data bank

  uint32_t iPtr=4;
  for(int iCh=0; iCh<mNCh; iCh++){
    uint32_t chSize = pZLEData[iPtr];
    uint32_t iChPtr = 1;// The chSize space is included in chSize
    uint32_t iBin=0;
    iPtr++;
    //std::cout << "--------------- Channel: " << aModule << "-" << iCh << " size=" << chSize << " " <<  std::endl;
    uint32_t prevGoodData=1;
    while(iChPtr<chSize){
      uint32_t goodData = ((pZLEData[iPtr]>>31) & 0x1);
      uint32_t nWords = (pZLEData[iPtr] & 0xFFFFF);
      if(prevGoodData==0 && goodData==0){ // consecutive skip. Bad
        /*  std::cout << "consecutive skip: V1720=" << aModule
      << " | ch=" << iCh
      << " | prev word="  << pZLEData[iPtr-1]
      << " | cur word=" << pZLEData[iPtr] << std::endl;
         */}
      prevGoodData=goodData;
      if(goodData){
        uint32_t iMin = iBin;
        uint32_t min=4096;
        uint32_t baseline = (pZLEData[iPtr+1]&0xFFF);
        for(uint32_t iWord=0; iWord<nWords; iWord++){
          iPtr++;
          iChPtr++;
          if(min > (pZLEData[iPtr]&0xFFF) ){
            iMin = iBin+iWord*2;
            min = (pZLEData[iPtr]&0xFFF);
          }
          if(min > ((pZLEData[iPtr]>>16)&0xFFF) ){
            iMin = iBin+iWord*2+1;
            min = ((pZLEData[iPtr]>>16)&0xFFF);
          }
        }
        // package channel | iMin | min in one 32 bit word
        // don't bother for now. Temporary!!!!
        (*nQt)++; // increment number of Qt
        min = baseline-min; // turn it into a positive number
        *QtData = (((mChMap[iCh]<<28) & 0xF0000000) |
            ((iMin<<16)        & 0x0FFF0000) |
            (min               & 0x0000FFFF));
        QtData++;
        //std::cout << aModule << " "  << mChMap[iCh] << " " << iMin << " " << min << " " << *(QtData-1) << std::endl;
      }
      else{  // skip
        iBin += (nWords*2);
      }

      iChPtr++;
      iPtr++;
    }
  }
  //std::cout << " | " << *nQt << std::endl;
  bk_close(pevent, QtData);
}

