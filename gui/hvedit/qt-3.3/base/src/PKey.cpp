/********************************************************************************************

  PKey.cpp

  class to handle a single midas ODB key.

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/25
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Id$

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

#include "PGlobals.h"
#include "PKey.h"

//**********************************************************************
/*!
 * <p>Constructs a PKey object, linked to a MIDAS experiment. It is
 * used to establish the main entry key for all the other ones.
 *
 * \param pExperiment Pointer to a MIDAS experiment object, i.e. the connection 
 *                    to this experiment is already established (see cmExperiment).
 * \param strKeyName  Name of the key in the ODB, which one wants to link to.
 */
PKey::PKey(PExperiment *pExperiment, QString *strKeyName)
{
  fExperiment = pExperiment;    // pointer to the experiment
  fHDB = pExperiment->GetHdb(); // handle to the ODB
  fValid = (db_find_key(fHDB, 0, (char *)strKeyName->ascii(), &fHKey) == DB_SUCCESS);
  if (fValid) {
    db_get_key(fHDB, fHKey, &fKey);
  } else {
    QString errMsg = QString("Cannot find %1 entry in online database").arg(*strKeyName);
    MidasMessage(errMsg.latin1());
  }
}

/*!
 * <p>Constructs a PKey object, linked to a MIDAS experiment. It is
 * used to establish a hot-link to a <em>specific</em> sub-key.
 *
 * \param pRootKey   Pointer to the root key.
 * \param strKeyName Name of the key in the ODB, which one wants to link to.
 */
PKey::PKey(PKey *pRootKey, QString *strKeyName)
{
  fExperiment = pRootKey->fExperiment; // pointer to the experiment
  fHDB = pRootKey->GetHdb();           // handle to the ODB
  fValid = (db_find_key(fHDB, pRootKey->GetHkey(), (char *)strKeyName->ascii(), &fHKey) == DB_SUCCESS);
  if (fValid) {
    db_get_key(fHDB, fHKey, &fKey);
  } else {
    QString errMsg = QString("Cannot find %1 entry in online database").arg(*strKeyName);
    MidasMessage(errMsg.latin1());
  }
}

//**********************************************************************
/*!
 * <p>Destroys the object.
 */
PKey::~PKey()
{
}

//**********************************************************************
/*!
 * <p>Updates the key handle and the related info structure.
 *
 * <p><b>Return:</b>
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid.
 */
INT PKey::UpdateKey()
{
  return db_get_key(fHDB, fHKey, &fKey);
}


//**********************************************************************
/*!
 * <p>Establishes a hot-link between a local data structure and the ODB, i.e.
 * everytime this structure is changed in the ODB, it is automatically
 * propageted to the structure and vice versa. Further is starts the cyclic
 * timer needed for the MIDAS watch-dog process.
 *
 * <p><b>Return:</b>
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid.
 *   - DB_STRUCT_SIZE_MISMATCH if the structure size doesn't match the sub-tree size
 *   - DB_NO_ACCESS if there is no READ/WRITE access to the sub-tree
 *
 * \param addr Address of the local structure to which a hot-link shall be established.
 * \param access Access flags for this hot-link.
 * \param dispatcher Callback function which is called, if the entries in the ODB have
 *                   changed.
 * \param obj Pointer to the calling object
 */
INT PKey::HotLink(void *addr, WORD access, void (*dispatcher)(INT,INT,void*), void *obj)
{
  fExperiment->StartTimer(); // start a cylcic timer event

  // establishes a hotlink
  return db_open_record(fHDB, fHKey, addr, fKey.total_size, access, dispatcher, obj);
}

//**********************************************************************
/*!
 * <p>Establishes a hot-link between a local data structure and the ODB, i.e.
 * everytime this structure is changed in the ODB, it is automatically
 * propageted to the structure and vice versa. Further is starts the cyclic
 * timer needed for the MIDAS watch-dog process.
 *
 * <p><b>Return:</b>
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid.
 *   - DB_STRUCT_SIZE_MISMATCH if the structure size doesn't match the sub-tree size
 *   - DB_NO_ACCESS if there is no READ/WRITE access to the sub-tree
 *
 * \param addr Address of the local structure to which a hot-link shall be established.
 * \param size size of the calling structure
 * \param access Access flags for this hot-link.
 * \param dispatcher Callback function which is called, if the entries in the ODB have
 *                   changed.
 * \param obj Pointer to the calling object
 */
INT PKey::HotLink(void *addr, int size, WORD access, void (*dispatcher)(INT,INT,void*), void *obj)
{
  fExperiment->StartTimer(); // start a cylcic timer event

  // establishes a hotlink
  return db_open_record(fHDB, fHKey, addr, size, access, dispatcher, obj);
}

//**********************************************************************
/*!
 * <p>Closes the hot-link, i.e. disconnects the tight link between the local
 * structure and the ODB.
 *
 * <p><b>Return:</b>
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid.
 */
INT PKey::Unlink()
{
  return db_close_record(fHDB, fHKey);
}

//**********************************************************************
/*!
 * <p>Enumerates keys in a ODB directory.
 *
 * <p><b>Return:</b>
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid.
 *   - DB_NO_MORE_SUBKEYS no more sub-keys available
 *
 * \param index of a possible subkey
 * \param subkey pointer to the subkey
 */
INT PKey::EnumSubKey(INT index, HNDLE *subkey)
{
  return db_enum_key(fHDB, fHKey, index, subkey);
}

//**********************************************************************
/*!
 * <p>The function returns single values or whole arrays which are contained in an ODB key.
 * Since the data buffer is of type void, no type checking can be performed by the compiler.
 * Therefore the type has to be explicitly supplied, which is checked against the type
 * stored in the ODB.
 *
 * <p><b>Return:</b>
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid.
 *   - DB_TRUNCATED if there is a size mismatch between the local structure and the ODB.
 *   - DB_TYPE_MISMATCH if there is a type discrepancy between the local structure and the ODB.
 *
 * \param data Pointer to the data structure. 
 * \param size Size of data buffer.
 * \param type Type of key, one of TID_xxx (see MIDAS documentation)
 */
INT PKey::GetData(void *data, INT *size, DWORD type)
{
  return db_get_data(fHDB, fHKey, data, size, type);
}

//**********************************************************************
/*!
 * <p>The function returns single value which is contained in an ODB key. 
 * Since the data buffer is of type void, no type checking can be performed by the compiler. 
 * Therefore the type has to be explicitly supplied, which is checked against the type 
 * stored in the ODB.
 * 
 * <p><b>Return:</b> 
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid. 
 *   - DB_TRUNCATED if there is a size mismatch between the local structure and the ODB.
 *   - DB_TYPE_MISMATCH if there is a type discrepancy between the local structure and the ODB.
 *
 * \param data  Pointer to the data structure. 
 * \param size  Size of data buffer.
 * \param index Array index, between zero and n-1 where n is the size of the array.
 * \param type  Type of key, one of TID_xxx (see MIDAS documentation)
 */
INT PKey::GetDataIndex(void *data, INT *size, INT index, DWORD type)
{
  return db_get_data_index(fHDB, fHKey, data, size, index, type);
}

INT PKey::GetRecord(void *data, INT *size)
{
  return db_get_record(fHDB, fHKey, data, size, 0);
}

//**********************************************************************
/*!
 * <p>Set key data from a handle. Adjust number of values if previous data has different size.
 *
 * <p><b>Return:</b> 
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid.
 *   - DB_TRUNCATED if there is a size mismatch between the local structure and the ODB.
 *
 * \param data Pointer to the data structure. 
 * \param num_values Number of data elements.
 * \param type Type of key, one of TID_xxx (see MIDAS documentation)
 */
INT PKey::SetData(void *data, INT num_values, DWORD type)
{
  return db_set_data(fHDB, fHKey, data, num_values*fKey.item_size, num_values, type);
}

//**********************************************************************
/*!
 * <p>Set key data for a key which contains an array of values. 
 * This function sets individual values of a key containing an array. 
 * If the index is larger than the array size, the array is extended and 
 * the intermediate values are set to zero.
 *
 * <p><b>Return:</b> 
 *   - DB_SUCCESS on successful completion.
 *   - DB_INVALID_HANDLE if ODB or key is invalid. 
 *   - DB_TRUNCATED if there is a size mismatch between the local structure and the ODB.
 *
 * \param data Pointer to the data structure. 
 * \param index Index of the data element.
 * \param type Type of key, one of TID_xxx (see MIDAS documentation)
 */
INT PKey::SetDataIndex(void *data, INT index, DWORD type)
{
  return db_set_data_index(fHDB, fHKey, data, fKey.item_size, index, type);
}

//**********************************************************************
/*!
 * <p>Returns the time in seconds, when the key was last time modified.
 *
 * <p><b>Return:</b> Time when key was last time modified. 
 */
DWORD PKey::GetKeyTime()
{
  DWORD delta;

  db_get_key_time(fHDB, fHKey, &delta);
  return delta;
}
