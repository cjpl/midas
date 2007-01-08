/*******************************************************************************************

  PPwd.cpp

  implementation of the virtual class 'PPwdBase' from the Qt designer
  (see 'PPwdBase.ui').

  GUI for the password handeling with midas.

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/25
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2006/01/31 15:12:07  nemu
  replaces cmPwd.cpp

  Revision 1.1  2004/09/30 09:23:34  suter_a
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

#include <qlineedit.h>
#include "PPwd.h"

//*******************************************************************************************************
//  default Constructor
//*******************************************************************************************************
PPwd::PPwd(char *password, QWidget *parent, const char *name,
           bool modal, WFlags fl) : PPwdBase(parent, name, modal, fl)
{
  fPwd = password;
}

//*******************************************************************************************************
//  default Destructor
//*******************************************************************************************************
PPwd::~PPwd()
{
}

//*******************************************************************************************************
// getPassword
//
// gets the password and closes the GUI.
//*******************************************************************************************************
void PPwd::GetPassword()
{
  strcpy(fPwd, (char *)PwdLineEdit->text().ascii());
  close();
}
