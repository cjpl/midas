/*******************************************************************************************

  Qt_Dlg_Connect.cpp

  implementation of the virtual class 'Qt_Dlg_Connect_Base' from the Qt designer
  (see 'Qt_Dlg_Connect_Base.ui')

  This GUI handels the pc-name request, displays the corresponding midas experiment list.

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/21
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2003/05/09 10:08:09  midas
  Initial revision

  Revision 1.2  2003/02/27 15:19:28  nemu
  fixed minor mistake in getExpList()


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
#include <qlineedit.h>
#include <qlistbox.h>

#include "midas.h"

#include "Qt_Connect.h"

//*******************************************************************************************************
//  default Constructor
//*******************************************************************************************************
Qt_Connect::Qt_Connect(cmExperiment *cmExp, QWidget *parent, const char *name,
                       bool modal, WFlags fl) : Qt_Connect_Base(parent, name, modal, fl)
{
  Qt_Connect::cmExp = cmExp; // links local experiment name to the global one
  host_lineEdit->setFocus(); // set the focus to the host name field
}

//*******************************************************************************************************
//  default Destructor
//*******************************************************************************************************
Qt_Connect::~Qt_Connect()
{
}

//*******************************************************************************************************
//  implement slot getExpList()
//
//  gets the experiment-list from host and displays it.
//*******************************************************************************************************
void Qt_Connect::getExpList()
{
  cmExp->SetHostName(host_lineEdit->text());        // gets the host name
  char exp_name_list[MAX_EXPERIMENT][NAME_LENGTH];  // list of all the available experiments
  int  i;

  // try to get list of experiments from host
  if (cm_list_experiments((char *)cmExp->GetHostName().ascii(), exp_name_list) != CM_SUCCESS) {
    QMessageBox::warning(this, "ERROR", "Could not connect to "+cmExp->GetHostName(),
                         QMessageBox::Ok, QMessageBox::Cancel);
    return;
  }

  // generate list of experiments for widget
  exp_listBox->clear(); // deletes a perhaps existing list
  i=0;
  while (strcmp(exp_name_list[i],"")) {
    exp_listBox->insertItem(exp_name_list[i],-1); // insert new item at the end of the list
    i++;
  }
  exp_listBox->setFocus();
}

//*******************************************************************************************************
//  implement slot gotExperiment()
//
//  gets the selected experiment from the list and terminates the GUI.
//*******************************************************************************************************
void Qt_Connect::gotExperiment()
{
  cmExp->SetExperimentName(exp_listBox->currentText());
  Qt_Connect::done(0);
}
