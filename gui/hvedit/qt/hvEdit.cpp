/********************************************************************************************

  hvEdit.cpp

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/19
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

/* Qt stuff */
#include <qapplication.h>

#include "Qt_hvEdit.h"

int main(int argc, char *argv[])
{
  // create administration class
  hvAdmin *hvA = new hvAdmin();
  // create experiment class
  cmExperiment *mExp = new cmExperiment("HVEdit");
  
  QApplication app(argc, argv); // create main Qt application entry point
  Qt_hvEdit    *dlg_hvEdit = new Qt_hvEdit(hvA, mExp);  // create Qt hvEdit Dialog

  dlg_hvEdit->show(); // show widget
  return app.exec();
}
