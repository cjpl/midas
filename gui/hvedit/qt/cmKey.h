/*******************************************************************************************

  cmKey.h

*********************************************************************************************

    begin                : Andreas Suter, 2002/12/02
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2003/05/09 10:08:09  midas
  Initial revision

  Revision 1.1.1.1  2003/02/27 15:24:37  nemu
  added log info


********************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _CMKEY_H
#define _CMKEY_H

#include <qstring.h>

#include "cmExperiment.h"

class cmKey
{
  // Construction
  public:
    cmKey(cmExperiment *pExperiment, QString *strKeyName);
    cmKey(cmKey *pRootKey, QString *strKeyName);
    ~cmKey();

  // Attributes
  private:
    HNDLE  m_hDB;
    HNDLE  m_hKey;
    BOOL   m_bValid;
    KEY    m_Key;
    cmExperiment *m_pExperiment;

  // Operations
  public:
    HNDLE   GetHdb()  { return m_hDB; };
    HNDLE   GetHkey() { return m_hKey; };
    BOOL    IsValid() { return m_bValid; };
    INT     GetType() { return m_Key.type; };
    INT     GetItemSize() { return m_Key.item_size; };
    WORD    GetAccessMode() { return m_Key.access_mode; };
    INT     GetNumValues() { return m_Key.num_values; };
    INT     UpdateKey();
    INT     HotLink(void *addr, WORD access, void (*dispatcher)(INT,INT,void*), void *window);
    INT     Unlink();

    INT     GetData(void *data, INT *size, DWORD type);
    INT     SetData(void *data, INT num_values, DWORD type);
    INT     SetDataIndex(void *data, INT index, DWORD type);
    DWORD   GetKeyTime();
};


#endif // _CMKEY_H
