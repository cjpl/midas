/*********************************************************************

  Name:         VPC6.h
  Created by:   Pierre-Andre Amaudruz

  Contents:      Routines for accessing the vpc6 Triumf board

  $Id$
*********************************************************************/
#ifndef __VPC6_INCLUDE_H__
#define __VPC6_INCLUDE_H__

#include "mvmestd.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VPC6_SUCCESS                    1 //
#define VPC6_PARAM_ERROR              100 // parameters error
#define VPC6_SR_RO         (WORD) (0x0000)
#define VPC6_CR_RW         (WORD) (0x0004)
#define VPC6_CMD_WO        (WORD) (0x000C)
#define VPC6_CFG_RW        (WORD) (0x0010)
#define VPC6_RBCK_RO       (WORD) (0x0110)

#define VPC6_ASD01         0
#define VPC6_ALL_BUCKEYE   0x555
#define VPC6_ALL_ASD       0x000
#define VPC6_3ASD_3BUCK    0x444
#define VPC6_3BUCK_3ASD    0x111
#define ALL_CHANNELS       -1


/*-------------------------------------------*/
  enum vpc6_ASDDataType {
    vpc6_asd_ch1_8 =0,
    vpc6_asd_ch9_16 =1,
  };

  typedef union {
    DWORD asdcfg;
    struct Entry1 {
      unsigned chipmode:1; // bit0 here
      unsigned channelmode:16;
      unsigned deadtime:3;
      unsigned wilkinsonRCurrent:3;
      unsigned wilkinsonIGate:4;
      unsigned hysteresisDAC:4;
      unsigned wilkinsonADC:1; // bit0 here
    } asdx1;
    struct Entry2 {
      unsigned wilkinsonADC:2; // bit2..1 here
      unsigned mainThresholdDAC:8;
      unsigned capInjCapSel:3;
      unsigned calMaskReg:8;
      unsigned notused:10;
    } asdx2;
  } vpc6_Reg;

/*-------------------------------------------*/
#define VPC6_BUCKEYE       1
#define VPC6_NORMAL        0x0
#define VPC6_SMALLCAP      0x1
#define VPC6_MEDIUMCAP     0x2
#define VPC6_LARGECAP      0x3
#define VPC6_EXTERNALCAP   0x4
#define VPC6_KILL          0x7


  int  vpc6_isPortBusy(MVME_INTERFACE *mvme, DWORD base, WORD port);
  void vpc6_PATypeWrite(MVME_INTERFACE *mvme, DWORD base, WORD data);
  int  vpc6_PortTypeRead(MVME_INTERFACE *mvme, DWORD base, WORD port);
  int  vpc6_PATypeRead(MVME_INTERFACE *mvme, DWORD base);
  int  vpc6_CfgLoad(MVME_INTERFACE *mvme, DWORD base, WORD port);
  int  vpc6_CfgRetrieve(MVME_INTERFACE *mvme, DWORD base, WORD port);
  int  vpc6_PortRegRBRead(MVME_INTERFACE *mvme, DWORD base, WORD port);
  int  vpc6_PortRegRead(MVME_INTERFACE *mvme, DWORD base, WORD port);
  void vpc6_PortDisplay(WORD type, WORD port, DWORD * reg);
  void vpc6_EntryPrint(WORD type, WORD chip, const vpc6_Reg * v);
  void vpc6_ASDRegSet(MVME_INTERFACE *mvme, DWORD base, WORD port, WORD reg, vpc6_Reg * Reg);
  void vpc6_ASDDefaultLoad(MVME_INTERFACE *mvme, DWORD base, WORD port);
  int  vpc6_ASDModeSet(MVME_INTERFACE *mvme, DWORD base, WORD port, int channel, int mode);
  int  vpc6_ASDThresholdSet(MVME_INTERFACE *mvme, DWORD base, WORD port, int threshold);
  int  vpc6_ASDHysteresisSet(MVME_INTERFACE *mvme, DWORD base, WORD port, float hysteresis);
  int  vpc6_BuckeyeModeSet(MVME_INTERFACE *mvme, DWORD base, WORD port, int channel,  int mode);

#ifdef __cplusplus
}
#endif

#endif
