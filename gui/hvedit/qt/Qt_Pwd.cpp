/*******************************************************************************************

  Qt_Pwd.cpp

  implementation of the virtual class 'Qt_Pwd_Base' from the Qt designer
  (see 'Qt_Pwd_Base.ui').

  GUI for the password handeling with midas.

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

#include <qlineedit.h>
#include "Qt_Pwd.h"

//*******************************************************************************************************
//  default Constructor
//*******************************************************************************************************
Qt_Pwd::Qt_Pwd(char *password, QWidget *parent, const char *name,
               bool modal, WFlags fl) : Qt_Pwd_Base(parent, name, modal, fl)
{
  pwd = password;
}

//*******************************************************************************************************
//  default Destructor
//*******************************************************************************************************
Qt_Pwd::~Qt_Pwd()
{
}

//*******************************************************************************************************
// getPassword
//
// gets the password and closes the GUI.
//*******************************************************************************************************
void Qt_Pwd::GetPassword()
{
  strcpy(pwd, (char *)pwd_lineEdit->text().ascii());
  close();
}
