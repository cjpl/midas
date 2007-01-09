/********************************************************************************************

  hvEdit.cpp

*********************************************************************************************

    begin                : Andreas Suter, 2007/01/08
    modfied:             :
    copyright            : (C) 2007 by
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

#include <iostream>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <qapplication.h>
#include <qstring.h>

#include "PHvEdit.h"

//-----------------------------------------------------------------------------
/*!
 * <p>Syntax message function.
 *
 * \param progName name of the calling program
 */
void hvEditSyntax(char *progName)
{
  if (strstr(progName, "./")) { // to eliminate ./ if present, e.g. './hvedit' -> 'hvedit'
    progName +=2;
  }
  cout << endl << endl << "SYNTAX: " << progName << " [--help] ¦ [-h host] [-e experiment]";
  cout << endl << "        --help results in this help";
  cout << endl << "        -h host is the host name of the MIDAS experiment";
  cout << endl << "        -e experiment is the experiment name";
  cout << endl << endl;
}

//-----------------------------------------------------------------------------
/*!
 * <p>hvEdit main function call
 */
int main(int argc, char *argv[])
{
  int  host_no = 0;
  int  exp_no  = 0;
  char host_name[256];
  char exp_name[256];
  QString str;

  // get default host- and experiment name
  if (getenv("HOSTNAME"))
    strcpy(host_name, getenv("HOSTNAME"));
  else
    strcpy(host_name, "");

  if (getenv("MIDAS_EXPT_NAME"))
    strcpy(exp_name, getenv("MIDAS_EXPT_NAME"));
  else
    strcpy(exp_name, "");
    
  // check argument list
  switch (argc) {
    case 1:
      break;
    case 2:
      hvEditSyntax(argv[0]);
      exit(-1);
    case 3:
      if (strstr(argv[1], "-h") || (strstr(argv[1], "-e"))) {
        if (strstr(argv[1], "-h"))
          host_no = 2;
        else
          exp_no  = 2;
      } else {
        hvEditSyntax(argv[0]);
        exit(-1);
      }
      break;
    case 5:
      if (strstr(argv[1], "-h") && (strstr(argv[3], "-e"))) {
        host_no = 2;
        exp_no  = 4;
      } else if (strstr(argv[1], "-e") && (strstr(argv[3], "-h"))) {
        host_no = 4;
        exp_no  = 2;
      } else {
        hvEditSyntax(argv[0]);
        exit(-1);
      }
      break;
    default:
      hvEditSyntax(argv[0]);
      exit(-1);
  }

  if (host_no != 0)
    strcpy(host_name, argv[host_no]);

  if (exp_no != 0)
    strcpy(exp_name, argv[exp_no]);

  if (strlen(host_name)==0) {
    cout << endl << "Cannot obtain host name! This needs either to be given explicitly,";
    cout << endl << "or the environment variable HOSTNAME needs to be set.";
    hvEditSyntax(argv[0]);
    exit(-1);
  }

  if (strlen(exp_name)==0) {
    // try to extract the experiment name directly from the ODB
    INT status;
    status = cm_connect_experiment1(host_name, exp_name, "hvEdit", NULL,
                                    DEFAULT_ODB_SIZE, DEFAULT_WATCHDOG_TIMEOUT);
    HNDLE hDB;
    status = cm_get_experiment_database(&hDB, NULL);
    INT  size = NAME_LENGTH;
    status = db_get_value(hDB, 0, "/Experiment/Name", exp_name, &size, TID_STRING, FALSE);
    cm_disconnect_experiment();
    if ((strlen(exp_name)==0) || (status != DB_SUCCESS)) { // failed to extract the name directly from the ODB 
      cout << endl << "Cannot obtain experiment name! This needs either to be given explicitly,";
      cout << endl << "or the environment variable MIDAS_EXPT_NAME needs to be set or at least ";
      cout << endl << "one MIDAS experiment needs to be defined.";
      cout << endl;
      hvEditSyntax(argv[0]);
      exit(-1);
    }
  }

  // host-, exp name to lower case 
  str = QString(host_name);
  strcpy(host_name, str.lower().latin1());
  str = QString(exp_name);
  strcpy(exp_name, str.lower().latin1());

  // create administration class
  PHvAdmin *hvA = new PHvAdmin(host_name, exp_name);
  // create experiment class
  PExperiment *mExp = new PExperiment("hvEdit");

  QApplication app(argc, argv); // create main Qt application entry point
  PHvEdit *dlg_hvEdit = new PHvEdit(hvA, mExp);  // create Qt hvEdit Dialog

  dlg_hvEdit->show(); // show widget

  QObject::connect(qApp, SIGNAL(lastWindowClosed()), dlg_hvEdit, SLOT(DisconnectAndQuit()));

  return app.exec();
}
