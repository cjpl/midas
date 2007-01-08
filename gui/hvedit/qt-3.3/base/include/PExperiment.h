/********************************************************************************************

  PExperiment.h

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

#ifndef _PEXPERIMENT_H_
#define _PEXPERIMENT_H_

#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>

#include "midas.h"

void GetPassword(char *password);

class PExperiment : public QObject {
  Q_OBJECT

  public:
    PExperiment(QString strClientName, QObject *parent = 0, const char *kName = 0);
    ~PExperiment();

  // Attributes
  private:
    QString fStrHostName;       //!< host name on which the experiment is running
    QString fStrExperimentName; //!< name of the experiment
    QString fStrClientName;     //!< name of the client calling PExperiment, e.g. 'hvEdit'
    BOOL    fConnected;         //!< flag showing if a connection to an experiment is established
    BOOL    fTimerStarted;      //!< flag showing if the cyclic timer is already running
    HNDLE   fHDB;               //!< main handle to the ODB
    QTimer *fTimer;             //!< pointer to a timer which is used for the watchdog process

  // Operations
  public:
   void    SetHostName(QString hostName) { fStrHostName = hostName; }; //!< Sets the name of the host
   void    SetExperimentName(QString expName) { fStrExperimentName = expName; }; //!< Sets the name of the experiment
   QString GetHostName() { return fStrHostName; }; //!< Returns the name of the host
   QString GetExperimentName() { return fStrExperimentName; }; //!< Returns the name of the MIDAS experiment to which one is connected
   QString GetClientName() { return fStrClientName; }; //!< Returns the name of the calling client
   HNDLE   GetHdb() { return fHDB; }; //!< Returns the main MIDAS handle to the ODB
   BOOL    IsConnected() { return fConnected; }; //!< Returns if already connected to an experiment
   BOOL    StartTimer();
   BOOL    Connect(QString host, QString experiment, QString client);
   void    Disconnect();

 // slots
 public slots:
   void FeedMidasWatchdog();

 signals:
   void NetworkConnectionLost();
   void ReceivedShutdownCmd();
};

#endif // _PEXPERIMENT_H_
