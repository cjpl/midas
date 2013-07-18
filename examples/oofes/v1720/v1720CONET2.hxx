/*****************************************************************************/
/**
\file v1720CONET2.hxx

## Contents

This file contains the class definition for the v1720 module driver.
 *****************************************************************************/

#ifndef V1720_HXX_INCLUDE
#define V1720_HXX_INCLUDE

#include <iostream>
#include <sstream>
#include <iomanip>
#include <assert.h>

extern "C" {
#include "CAENComm.h"
#include "ov1720drv.h"
}
#include "midas.h"

/**
 * Driver class for the v1720 module using the CAEN CONET2 (optical) interface.
 * Contains all the methods necessary to:
 *
 * - Connect/disconnect the board through an optical connection
 * - Initialize the hardware (set the registers) for data acquisition
 * - Read and write to the ODB
 * - Poll the hardware and read the event buffer into a midas bank
 * - Handle ZLE data
 * - Send a software trigger to the board if desired
 */
class v1720CONET2
{
private:
#if SIMULATION
  int _handle, _board;
#else
  int _handle,  //!< Device handler
  _feindex,     //!< Frontend index number
  _link,        //!< Optical link number
  _board,       //!< Module/Board number
  _moduleID;    //!< Unique module ID
#endif

  HNDLE _odb_handle;      //!< ODB handle
  bool _settings_loaded,  //!< ODB settings loaded
  _settings_touched,      //!< ODB settings touched
  _running;               //!< Run in progress

public:
  int verbose;  //!< Make the driver verbose
  //!< 0: off
  //!< 1: normal
  //!< 2: very verbose


  //! Settings structure for this v1740 module
  struct V1720_CONFIG_SETTINGS {
    INT       setup; //!< Initial board setup mode number
    INT       acq_mode;                //!< 0x8100@[ 1.. 0]
    DWORD     channel_config;          //!< 0x8000@[19.. 0]
    INT       buffer_organization;     //!< 0x800C@[ 3.. 0]
    INT       custom_size;             //!< 0x8020@[31.. 0]
    DWORD     channel_mask;            //!< 0x8120@[ 7.. 0]
    DWORD     trigger_source;          //!< 0x810C@[31.. 0]
    DWORD     trigger_output;          //!< 0x8110@[31.. 0]
    DWORD     post_trigger;            //!< 0x8114@[31.. 0]
    // Hard code the two fp_* settings to alway on (Alex 21/2/13)
    //    DWORD     fp_io_ctrl;              //!< 0x811C@[31.. 0]
    DWORD     almost_full;             //!< 0x816C@[31.. 0]
    //    DWORD     fp_lvds_io_ctrl;         //!< 0x81A0@[31.. 0]
    //    BOOL      trigger_positive_pulse;  //!< 0x8000@[     6]
    //    WORD      pre_record_zs;           //!< 0x1n24@[31..16]
    //    WORD      post_record_zs;          //!< 0x1n24@[15.. 0]
    DWORD     threshold[8];            //!< 0x1n80@[11.. 0]
    DWORD     nbouthreshold[8];        //!< 0x1n84@[11.. 0]
    INT       zs_threshold[8];         //!< 0x1n24@[31.. 0]
    DWORD     zs_nsamp[8];             //!< 0x1n28@[31.. 0]
    DWORD     dac[8];                  //!< 0x1n98@[15.. 0]
  } config; //!< instance of config structure
  static const char *config_str[];

#if SIMULATION
  v1720CONET2(int board);
#else
  v1720CONET2(int feindex, int link, int board, int moduleID);
#endif
  ~v1720CONET2();

  CAENComm_ErrorCode Connect();
  CAENComm_ErrorCode Disconnect();
  CAENComm_ErrorCode StartRun();
  CAENComm_ErrorCode StopRun();
  CAENComm_ErrorCode SetupPreset(int);
  CAENComm_ErrorCode AcqCtl(uint32_t);
  CAENComm_ErrorCode ChannelConfig(uint32_t);
  CAENComm_ErrorCode ReadReg(DWORD, DWORD*);
  CAENComm_ErrorCode WriteReg(DWORD, DWORD);
  CAENComm_ErrorCode ReadEvent(DWORD*, int*);
  CAENComm_ErrorCode SendTrigger();
  CAENComm_ErrorCode Poll(DWORD*);

  int SetODBRecord(HNDLE h, void(*cb_func)(INT,INT,void*));
  int InitializeForAcq();
  bool IsConnected();
  bool IsRunning();
  std::string GetName();
  HNDLE GetODBHandle() { return _odb_handle; }  //!< returns ODB handle
  bool GetSettingsTouched() {                   //! returns true if odb settings  touched
    return _settings_touched;
  }
  void SetSettingsTouched(bool t) {             //! set _settings_touched
    _settings_touched = t;
  }
  int GetHandle() { return _handle; }           //!< returns device handler
#if SIMULATION
  int GetBoard() { return _board; }             //!< returns board number
  int GetModuleID() { return _board; }          //!< returns board number
#else
  int GetModuleID() { return _moduleID; }       //!< returns unique module ID
  int GetLink() { return _link; }               //!< returns optical link number
  int GetBoard() { return _board; }             //!< returns board number
  int GetFEIndex() { return _feindex; }         //!< returns frontend index
#endif

  //Utilities
  //  int mNCh;
  //  uint32_t mChMap[8];
  BOOL mZLE; //!< true if ZLE (Zero-length encoding) is enabled on all channels
  /**
   * Data type for all channels:
   * - 0: Full data, 2 packing
   * - 1: Full data, 2.5 packing
   * - 2: ZLE data, 2 packing
   * - 3: ZLE data, 2.5 packing */
  int mDataType;
  void getChannelConfig(DWORD aChannelConfig);
  BOOL IsZLEData();
  void fillQtBank(char * aDest, uint32_t * aZLEData, int aModule);
};

#endif // V1720_HXX_INCLUDE
