/********************************************************************************************

   Qt_Dlg_Connect.h

   implementation of the virtual class 'Qt_Dlg_Connect_Base' from the Qt designer
   (see 'Qt_Dlg_Connect_Base.ui').

   For details see the implementation: Qt_Dlg_Connect.cpp

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/21
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
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

#ifndef QT_CONNECT_H
#define QT_CONNECT_H

#include "cmExperiment.h"
#include "Qt_Connect_Base.h"

class Qt_Connect : public Qt_Connect_Base
{
  Q_OBJECT

  // Constructor/Destructor
  public:
    Qt_Connect(cmExperiment *cmExp,
               QWidget *parent = 0, const char *name = 0, bool modal = FALSE, WFlags f = WDestructiveClose);
    ~Qt_Connect();

  // Attributes
  private:
    cmExperiment *cmExp;

  // Slots
  public slots:
    void gotExperiment();

  private slots:
    void getExpList();
};

#endif // QT_CONNECT_H


