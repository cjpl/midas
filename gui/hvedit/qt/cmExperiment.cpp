/*******************************************************************************************

  cmExperiment.cpp

  class to connect to a midas experiment

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

#include "Qt_Pwd.h"
#include "globals.h"

#include "cmExperiment.h"

//**********************************************************************
// get Password callback routine -- CANNOT be a class member since
// the c-calling midas routine needs a pointer to void which cannot
// be accomplished be casting!
//**********************************************************************
void GetPassword(char *password)
{
  // create password dialog
  Qt_Pwd *pwd = new Qt_Pwd(password);
  // execute password dialog
  pwd->exec();
}

//**********************************************************************
// Constructor
//**********************************************************************
cmExperiment::cmExperiment(QString strClientName, QObject *parent = 0,
                           const char *name = 0) : QObject(parent, name)
{
  // Set empty strings
  m_strHostName       = "";
  m_strExperimentName = "";
  m_strClientName     = strClientName;
  
  m_bConnected = FALSE;
}

//**********************************************************************
// Destructor
//**********************************************************************
cmExperiment::~cmExperiment()
{
}

//**********************************************************************
// Connect
// 
// connects to a midas experiment.
//**********************************************************************
BOOL cmExperiment::Connect(QString strHostName, QString strExperimentName, QString strClientName)
{
  int status;

  // initialize variables
  m_bConnected    = FALSE;
  m_bTimerStarted = FALSE;

  // try to connect to midas host
  do {
    status = cm_connect_experiment1((char *)strHostName.ascii(),
                                    (char *)strExperimentName.ascii(),
                                    (char *)strClientName.ascii(), GetPassword,
                                    DEFAULT_ODB_SIZE, 20000); // increased timeout for W95

    if (status == CM_WRONG_PASSWORD) {
      MidasMessage("Password is incorrect");
      return FALSE;
    }

    if (status != CM_SUCCESS) { // midas error
      char str[256];

      cm_get_error(status, str);
      MidasMessage(str);
    }

  } while (status != CM_SUCCESS);

  // disable watchdog during debugging
#ifdef _DEBUG
  cm_set_watchdog_params(TRUE, 0);
#endif

  // route all messages to MidasMessage
  cm_set_msg_print(MT_ALL, MT_ALL, MidasMessage);

  // get handle to ODB
  cm_get_experiment_database(&m_hDB, NULL);

  // set connected flag
  m_bConnected = TRUE;

  return TRUE;
}

//**********************************************************************
// Disconnect
//
// disconnect from midas experiment.
//**********************************************************************
void cmExperiment::Disconnect()
{
  cm_disconnect_experiment();
  m_bConnected = FALSE;
}

//**********************************************************************
// StartTimer
//
// since all midas clients cyclically have to tell that they are still
// alive, such a cyclical timer is needed, which is implemented here.
// This cyclic timer works the following way: Once it is started, it
// runs in the background till timeout is reached. At timeout it calls
// a time out routine (timeOutProc) and the whole things starts all over
// again.
//**********************************************************************
BOOL cmExperiment::StartTimer()
{
  if (m_bTimerStarted)
    return TRUE;

  // create timer
  timer = new QTimer();

  // connect timer to event handler
  connect(timer, SIGNAL(timeout()), SLOT(timeOutProc()));

  // start timer
  if (!timer->start(500)) {
    MidasMessage("Cannot set timer");
    return FALSE;
  }

  m_bTimerStarted = TRUE;

  return TRUE;
}

//**********************************************************************
// slot timeOutProc
// 
// This routine is the time out routine of StartTimer and is cyclically 
// called. It self tells midas that hvEdit is still alive and checks
// if midas itself is alive and if midas (for what ever reason) kick
// hvEdit out.
//**********************************************************************
void cmExperiment::timeOutProc()
{
  int status;

  // tell MIDAS that hvEdit is still alive
  status = cm_yield(0);

  // check if MIDAS itself is still alive
  // check further if MIDAS did not through hvEdit out
  if (status == SS_ABORT || status == RPC_SHUTDOWN) {
    MidasMessage("Lost connection to the host");
  }
}
