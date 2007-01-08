/*******************************************************************************************

  PKey.h

*********************************************************************************************

    begin                : Andreas Suter, 2002/12/02
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2006/01/31 15:00:56  nemu
  replaces cmKey.h

  Revision 1.2  2005/05/03 06:41:50  nemu
  added GetDataIndex()

  Revision 1.1  2004/09/24 13:22:02  suter_a
  newly added



********************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _PKEY_H_
#define _PKEY_H_

#include <qstring.h>

#include "PExperiment.h"

class PKey {
   // Construction
 public:
   PKey(PExperiment *pExperiment, QString *strKeyName);
   PKey(PKey *pRootKey, QString *strKeyName);
   ~PKey();

   // Attributes
 private:
   HNDLE fHDB;    //!< Main handle to the ODB of MIDAS.
   HNDLE fHKey;   //!< Handle to a key in the ODB.
   BOOL  fValid;  //!< Validator flag, showing if the searched key is accessible or existing.
   KEY   fKey;    //!< MIDAS Structure to the key. Holding informations about the content of the key.
   PExperiment *fExperiment; //!< Pointer to a MIDAS experiment object.

   // Operations
 public:
   HNDLE GetHdb() { return fHDB; };     //!< Returns the Main handle of the ODB.
   HNDLE GetHkey() { return fHKey; };   //!< Returns the key.
   BOOL  IsValid() { return fValid; };   //!< Returns if the searched key is accessible or existing.
   INT   GetType() { return fKey.type; }; //!< Returns the type of the key.
   INT   GetItemSize() { return fKey.item_size; }; //!< Returns the item size of the key.
   WORD  GetAccessMode() { return fKey.access_mode; }; //!< Returns the access mode of the key
   INT   GetNumValues() { return fKey.num_values; }; //!< Returns the number of elements contained in the key.
   INT   UpdateKey();
   INT   HotLink(void *addr, WORD access, void (*dispatcher) (INT, INT, void *), void *obj);
   INT   HotLink(void *addr, int size, WORD access, void (*dispatcher) (INT, INT, void *), void *obj);
   INT   Unlink();

   INT   EnumSubKey(INT index, HNDLE *subKey);

   INT   GetData(void *data, INT *size, DWORD type);
   INT   GetDataIndex(void *data, INT *size, INT index, DWORD type);
   INT   GetRecord(void *data, INT *size);
   INT   SetData(void *data, INT num_values, DWORD type);
   INT   SetDataIndex(void *data, INT index, DWORD type);
   DWORD GetKeyTime();
};

#endif // _PKEY_H_
