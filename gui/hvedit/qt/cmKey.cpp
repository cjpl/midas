/********************************************************************************************

  cmKey.cpp

  class to handle a single midas ODB key.

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/25
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2003/05/09 10:08:09  midas
  Initial revision

  Revision 1.1.1.1  2003/02/27 15:26:12  nemu
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

#include <qmessagebox.h>

#include "midas.h"

#include "cmKey.h"

//**********************************************************************
// Constructor
//**********************************************************************
cmKey::cmKey(cmExperiment *pExperiment, QString *strKeyName)
{
  m_pExperiment = pExperiment;   // pointer to the experiment
  m_hDB = pExperiment->GetHdb(); // handle to the ODB
  m_bValid = (db_find_key(m_hDB, 0, (char *)strKeyName->ascii(), &m_hKey) == DB_SUCCESS);
  if (m_bValid) {
    db_get_key(m_hDB, m_hKey, &m_Key);
  } else {
    QMessageBox::warning(0, "MIDAS", "Cannot find " + *strKeyName + " entry in online database",
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}

cmKey::cmKey(cmKey *pRootKey, QString *strKeyName)
{
  m_pExperiment = pRootKey->m_pExperiment; // pointer to the experiment
  m_hDB = pRootKey->GetHdb();              // handle to the ODB
  m_bValid = (db_find_key(m_hDB, pRootKey->GetHkey(),
                          (char *)strKeyName->ascii(), &m_hKey) == DB_SUCCESS);
  if (m_bValid) {
    db_get_key(m_hDB, m_hKey, &m_Key);
  } else {
    QMessageBox::warning(0, "MIDAS", "Cannot find " + *strKeyName + " entry in online database",
                         QMessageBox::Ok, QMessageBox::NoButton);
  }
}

//**********************************************************************
// Destructor
//**********************************************************************
cmKey::~cmKey()
{
}

//**********************************************************************
// UpdateKey
//**********************************************************************
INT cmKey::UpdateKey()
{
  return db_get_key(m_hDB, m_hKey, &m_Key);
}


//**********************************************************************
// HotLink
//
// establishes a hotlink between a local data structure and the ODB, i.e.
// everytime this structure is changed in the ODB, it is automatically
// propageted to the structure and vice versa.
//**********************************************************************
INT cmKey::HotLink(void *addr, WORD access, void (*dispatcher)(INT,INT,void*), void *window)
{
  m_pExperiment->StartTimer(); // start a cylcic timer event

  // establishes a hotlink
  return db_open_record(m_hDB, m_hKey, addr, m_Key.total_size, access, dispatcher, window);
}

//**********************************************************************
// Unlink
//
// close the hotlink
//**********************************************************************
INT cmKey::Unlink()
{
  return db_close_record(m_hDB, m_hKey);
}

//**********************************************************************
// GetData
//
// get the data from the ODB
//**********************************************************************
INT cmKey::GetData(void *data, INT *size, DWORD type)
{
  return db_get_data(m_hDB, m_hKey, data, size, type);
}

//**********************************************************************
// SetData
//
// set the data in the ODB
//**********************************************************************
INT cmKey::SetData(void *data, INT num_values, DWORD type)
{
  return db_set_data(m_hDB, m_hKey, data, num_values*m_Key.item_size, num_values, type);
}

//**********************************************************************
// SetDataIndex
//
// set the data in the ODB with an index
//**********************************************************************
INT cmKey::SetDataIndex(void *data, INT index, DWORD type)
{
  return db_set_data_index(m_hDB, m_hKey, data, m_Key.item_size, index, type);
}

//**********************************************************************
// GetKeyTime
//**********************************************************************
DWORD cmKey::GetKeyTime()
{
  DWORD delta;

  db_get_key_time(m_hDB, m_hKey, &delta);
  return delta;
}
