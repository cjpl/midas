/*******************************************************************************************

  Qt_Pwd.h

  implementation of the virtual class 'Qt_Pwd_Base' from the Qt designer
  (see 'Qt_Pwd_Base.ui').

  For details see the implementation: Qt_Pwd.cpp

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/25
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.2  2003/12/30 14:54:26  suter_a
  "doxygenized" code, i.e. added comments which can be handled by doxygen in
  order to generate html- and latex-docu.

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

#ifndef QT_PWD_H
#define QT_PWD_H

#include "Qt_Pwd_Base.h"

class Qt_Pwd : public Qt_Pwd_Base
{
  Q_OBJECT

  // Constructor
  public:
    Qt_Pwd(char *password, QWidget *parent = 0, const char *name = 0, bool modal = FALSE, WFlags f = WDestructiveClose);
    ~Qt_Pwd();

  // Attributes
  private:
    char *pwd; //!< holds the password string

  // Slots
  public slots:
    void GetPassword();
};

#endif // QT_PWD_H
