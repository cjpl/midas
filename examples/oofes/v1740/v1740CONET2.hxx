/*****************************************************************************/
/**
\file v1740CONET2.hxx

## Contents

This file contains the class definition for the v1740 module driver.
 *****************************************************************************/

#ifndef V1740_HXX_INCLUDE
#define V1740_HXX_INCLUDE

#include <iostream>
#include <sstream>
#include <iomanip>

extern "C" {
#include "CAENComm.h"
#include "ov1740drv.h"
}
#include "midas.h"

/**
 * Driver class for the v1740 module using the CAEN CONET2 (optical) interface.
 * Contains all the methods necessary to:
 *
 * - Connect/disconnect the board through an optical connection
 * - Initialize the hardware (set the registers) for data acquisition
 * - Read and write to the ODB
 * - Poll the hardware and read the event buffer into a midas bank
 * - Send a software trigger to the board if desired
 */

class v1740CONET2
{
private:
  int _handle,  //!< Device handler
  _link,        //!< Optical link number
  _board,       //!< Module/Board number
  _moduleID;    //!< Unique module ID

  HNDLE _odb_handle;
  bool _settings_loaded,  //!< ODB settings loaded
  _settings_touched,      //!< ODB settings touched
  _running;               //!< Run in progress
public:
  int verbose;  //!< Make the driver verbose
  //!< 0: off
  //!< 1: normal
  //!< 2: very verbose
  //  bool _settings_touched;

  //! Settings structure for this v1740 module
  struct V1740_CONFIG_SETTINGS {
    INT       setup; //!< Initial board setup mode number
    INT       acq_mode;               //!< 0x8100@[ 1.. 0]
    DWORD     group_config;           //!< 0x8000@[19.. 0]
    INT       buffer_organization;    //!< 0x800C@[ 3.. 0]
    INT       custom_size;            //!< 0x8020@[31.. 0]
    DWORD     group_mask;             //!< 0x8120@[ 7.. 0]
    DWORD     trigger_source;         //!< 0x810C@[31.. 0]
    DWORD     trigger_output;         //!< 0x8110@[31.. 0]
    DWORD     post_trigger;           //!< 0x8114@[31.. 0]
    DWORD     threshold[8];           //!< 0x1n80@[11.. 0]
    DWORD     dac[8];                 //!< 0x1n98@[15.. 0]
  } config; //!< instance of config structure
  static const char *config_str[];

  v1740CONET2(int link, int board, int moduleID);
  ~v1740CONET2();

  CAENComm_ErrorCode Connect();
  CAENComm_ErrorCode Disconnect();
  CAENComm_ErrorCode StartRun();
  CAENComm_ErrorCode StopRun();
  CAENComm_ErrorCode SetupPreset(int);
  CAENComm_ErrorCode AcqCtl(uint32_t);
  CAENComm_ErrorCode ReadReg(DWORD, DWORD*);
  CAENComm_ErrorCode WriteReg(DWORD, DWORD);
  CAENComm_ErrorCode ReadEvent(DWORD*, int*);
  CAENComm_ErrorCode SendTrigger();

  int SetODBRecord(HNDLE h, void(*cb_func)(INT,INT,void*));
  int InitializeForAcq();
  bool IsConnected();
  bool IsRunning();
  std::string GetName();
  void CheckBoardType();
  void ShowRegs();
  HNDLE GetODBHandle() { return _odb_handle; }  //!< returns ODB handle
  bool GetSettingsTouched() {                   //! returns true if odb settings  touched
    return _settings_touched;
  }
  void SetSettingsTouched(bool t) {             //! set _settings_touched
    _settings_touched = t;
  }
  int GetHandle() { return _handle; }           //!< returns device handler
  int GetModuleID() { return _moduleID; }       //!< returns unique module ID
  int GetLink() { return _link; }               //!< returns optical link number
  int GetBoard() { return _board; }             //!< returns board number

};


#endif // V1740_HXX_INCLUDE
