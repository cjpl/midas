/*********************************************************************

  Name:         v513.h
  Created by:   K.Olchanski

  Contents:     CAEN V513 16-channel NIM I/O register

  $Id$
*********************************************************************/

#ifndef  V513_INCLUDE_H
#define  V513_INCLUDE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mvmestd.h"

#define V513_CHANMODE_OUTPUT    0
#define V513_CHANMODE_INPUT     1
#define V513_CHANMODE_NEG       0
#define V513_CHANMODE_POS       2
#define V513_CHANMODE_IGLITCHED 0
#define V513_CHANMODE_INORMAL   4
#define V513_CHANMODE_TRANSP    0
#define V513_CHANMODE_EXTSTROBE 8

uint16_t v513_RegisterRead(MVME_INTERFACE *mvme, DWORD base, int offset);
void     v513_RegisterWrite(MVME_INTERFACE *mvme, DWORD base, int offset, uint16_t value);
uint16_t v513_Read(MVME_INTERFACE *mvme, DWORD base);
void     v513_Write(MVME_INTERFACE *mvme, DWORD base, uint16_t data);
void     v513_Reset(MVME_INTERFACE *mvme, DWORD base);
void     v513_Status(MVME_INTERFACE *mvme, DWORD base);
void     v513_SetChannelMode(MVME_INTERFACE *mvme, DWORD base, int channel, int mode); // mode are V513_CHANMODE_XXX bits
int      v513_GetChannelMode(MVME_INTERFACE *mvme, DWORD base, int channel);

#endif // V513_INCLUDE_H
