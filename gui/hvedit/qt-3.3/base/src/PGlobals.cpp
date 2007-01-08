/********************************************************************************************

  PGlobals.cpp

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

#include "midas.h"
#include <qmessagebox.h>

//****************************************************************************************
/*!
 * <p> MidasMessage translates midas error messages to Qt Dialogs. This only is done, when
 * the compiler flag <tt>QT_MIDAS_MSG</tt> is set. In the qmake file this can be done by
 * adding <code>DEFINES += QT_MIDAS_MSG</code> to the .pro file.
 *
 * <p><b>return:</b> 0
 * \param str error message.
 */
int MidasMessage(const char *str)
{
  char name[NAME_LENGTH];

  cm_get_client_info(name);

  cm_msg(MINFO, name, str);

#ifdef QT_MIDAS_MSG
  QMessageBox::warning(0, "MIDAS ERROR", str, QMessageBox::Ok, QMessageBox::NoButton);
#endif

  return 0;
}

