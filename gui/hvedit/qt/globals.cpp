/********************************************************************************************

  globals.cpp

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

#include "midas.h"
#include <qmessagebox.h>

//****************************************************************************************
//
// MidasMessage translates midas error messages to Qt Dialogs.
//
//****************************************************************************************

int MidasMessage(const char *str)
{
  char name[NAME_LENGTH];
  
  cm_get_client_info(name);
  QMessageBox::warning(0, "MIDAS ERROR", str, QMessageBox::Ok, QMessageBox::NoButton);
  return 0;
}

