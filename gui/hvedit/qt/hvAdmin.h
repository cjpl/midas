/********************************************************************************************

  hvAdmin.h

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/22
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2003/05/09 10:08:09  midas
  Initial revision

  Revision 1.1.1.2  2003/03/03 09:33:50  nemu
  add documentation path info to help facility.

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

#ifndef _HVADMIN_H
#define _HVADMIN_H

#include <qstring.h>

#include "midas.h"

class hvAdmin
{
  // construct
  public:
    hvAdmin();
    ~hvAdmin();

  // attributes
  private:
    QString hvA_default_dir_hv_settings;
    QString hvA_default_dir_docu;
    QString hvA_midas_odb_hv_root;
    QString hvA_midas_odb_hv_names;
    QString hvA_midas_odb_hv_demand;
    QString hvA_midas_odb_hv_measured;
    QString hvA_midas_odb_hv_current;
    QString hvA_midas_odb_hv_current_limit;
    BOOL    b_hvA_default_dir_hv_settings;
    BOOL    b_hvA_default_dir_docu;
    BOOL    b_hvA_midas_odb_hv_root;
    BOOL    b_hvA_midas_odb_hv_names;
    BOOL    b_hvA_midas_odb_hv_demand;
    BOOL    b_hvA_midas_odb_hv_measured;
    BOOL    b_hvA_midas_odb_hv_current;
    BOOL    b_hvA_midas_odb_hv_current_limit;
    
  // operations
  public:
    QString *getHVDefaultDir() { return &hvA_default_dir_hv_settings; };
    QString *getHVDefaultDoc() { return &hvA_default_dir_docu; };
    QString *getHV_ODB_Root() { return &hvA_midas_odb_hv_root; };
    QString *getHV_ODB_Names() { return &hvA_midas_odb_hv_names; };
    QString *getHV_ODB_Demand() { return &hvA_midas_odb_hv_demand; };
    QString *getHV_ODB_Measured() { return &hvA_midas_odb_hv_measured; };
    QString *getHV_ODB_Current() { return &hvA_midas_odb_hv_current; };
    QString *getHV_ODB_Current_Limit() { return &hvA_midas_odb_hv_current_limit; };
};

#endif // _HVADMIN_H
