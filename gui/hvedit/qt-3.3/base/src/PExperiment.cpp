/*******************************************************************************************

  PExperiment.cpp

  class to connect to a midas experiment

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

#include <iostream>
using namespace std;


#include "PPwd.h"
#include "PGlobals.h"

#include "PExperiment.h"

//**********************************************************************
/*!
 * <p>get password callback routine - <em>CANNOT</em> be a class member 
 * since the c-calling midas routine needs a pointer to void which cannot
 * be accomplished be casting!
 *
 * <p><b>return:</b> void
 * \param password is a pointer to the password variable.
 */
void GetPassword(char *password)
{
  // create password dialog
  PPwd *pwd = new PPwd(password);
  // execute password dialog
  pwd->exec();
}

//**********************************************************************
/*!
 * <p>Creates an object to a MIDAS experiment.
 *
 * \param strClientName Name of the client which connects to the experiment, e.g. 'hvEdit'
 * \param parent The parent of an object may be viewed as the object's owner. Setting 
 *               <em>parent</em> to 0 constructs an object with no parent. If the object 
 *               is a widget, it will become a top-level window.  
 * \param name   Name of the object. The object name is some text that can be used to 
 *               identify a QObject.
 */
PExperiment::PExperiment(QString strClientName, QObject *parent,
                         const char *name) : QObject(parent, name)
{
  // Set empty strings
  fStrHostName       = "";
  fStrExperimentName = "";
  fStrClientName     = strClientName;

  fConnected = FALSE;
}

//**********************************************************************
/*!
 * <p>Destroys the object.
 */
PExperiment::~PExperiment()
{
  if (fTimer) {
    delete fTimer;
    fTimer = 0;
  }
}

//**********************************************************************
/*!
 * <p>Establishes the connection to a MIDAS experiment, running on the
 * host strHostName.
 *
 * <p><b>return:</b> 
 *   - TRUE, if connection to the experiment is established
 *   - FALSE, otherwise
 *
 * \param strHostName Name of the host on which the MIDAS experiment runs.
 * \param strExperimentName Name of the MIDAS experiment to which one wants to connect
 * \param strClientName Name of the client which connects to the experiment, e.g. 'hvEdit'
 */
BOOL PExperiment::Connect(QString strHostName, QString strExperimentName, QString strClientName)
{
  int status;
  
  // initialize variables
  fConnected    = FALSE;
  fTimerStarted = FALSE;

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
    return FALSE;
  }

  // disable watchdog during debugging
#ifdef _DEBUG
  cm_set_watchdog_params(TRUE, 0);
#endif

  // route all messages to MidasMessage
  cm_set_msg_print(MT_ALL, MT_ALL, MidasMessage);

  // get handle to ODB
  cm_get_experiment_database(&fHDB, NULL);

  // set connected flag
  fConnected = TRUE;

  return TRUE;
}

//**********************************************************************
/*!
 * <p>Disconnects from an already connected MIDAS experiment.
 *
 * <p><b>return:</b> void
 */
void PExperiment::Disconnect()
{
  cm_disconnect_experiment();
  fConnected = FALSE;
}

//**********************************************************************
/*!
 * <p>Since all MIDAS clients cyclically have to tell that they are still
 * alive, such a cyclical timer is needed, which is implemented here.
 * This cyclic timer works the following way: Once it is started, it
 * runs in the background till timeout is reached. At timeout it calls
 * a time out routine (timeOutProc) and the whole things starts all over
 * again.
 *
 * <p><b>return:</b>
 *   - TRUE if timer could have been started
 *   - FALSE otherwise
 */
BOOL PExperiment::StartTimer()
{
  if (fTimerStarted)
    return TRUE;

  // create timer
  fTimer = new QTimer();

  // connect timer to event handler
  connect(fTimer, SIGNAL(timeout()), SLOT(FeedMidasWatchdog()));

  // start timer
  if (!fTimer->start(500)) {
    MidasMessage("Cannot set timer");
    return FALSE;
  }

  fTimerStarted = TRUE;

  return TRUE;
}

//**********************************************************************
/*!
 * <p> This routine is the time out routine of StartTimer and is cyclically
 * called. It self tells MIDAS that it is still alive and checks
 * if MIDAS itself is alive and/or if MIDAS (for what ever reason) kick
 * to calling process out.
 *
 * <p><b>return:</b> void
 */
void PExperiment::FeedMidasWatchdog()
{
  int status;

  // tell MIDAS that hvEdit is still alive
  status = cm_yield(0);

  // network connection broken
  if (status == SS_ABORT) {
    emit NetworkConnectionLost();
  }

  // process was shutdown by another midas process (timeout)
  if (status == RPC_SHUTDOWN) {
    emit ReceivedShutdownCmd();
  }
}

//**********************************************************************
// end
//**********************************************************************
