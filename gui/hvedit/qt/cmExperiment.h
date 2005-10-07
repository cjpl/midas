/********************************************************************************************

  cmExperiment.h

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/25
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Id:$

********************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _CMEXPERIMENT_H
#define _CMEXPERIMENT_H

#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>

#include "midas.h"

void GetPassword(char *password);

class cmExperiment:public QObject {
   Q_OBJECT
       // Construction
 public:
   cmExperiment(QString strClientName, QObject * parent = 0, const char *name = 0);
   ~cmExperiment();

   // Attributes
 private:
    QString m_strHostName;      //!< host name on which the experiment is running
   QString m_strExperimentName; //!< name of the experiment
   QString m_strClientName;     //!< name of the client calling cmExperiment, e.g. 'hvEdit'
   BOOL m_bConnected;           //!< flag showing if a connection to an experiment is established  
   BOOL m_bTimerStarted;        //!< flag showing if the cyclic timer is already running
   HNDLE m_hDB;                 //!< main handle to the ODB
   QTimer *timer;               //!< pointer to a timer which is used for the watchdog process

   // Operations
 public:
   //! Sets the name of the host
   void SetHostName(QString hostName) {
      m_strHostName = hostName;
   };
   //! Sets the name of the experiment
   void SetExperimentName(QString expName) {
      m_strExperimentName = expName;
   };
   //! Returns the name of the host
   QString GetHostName() {
      return m_strHostName;
   };
   //! Returns the name of the MIDAS experiment to which one is connected
   QString GetExperimentName() {
      return m_strExperimentName;
   };
   //! Returns the name of the calling client
   QString GetClientName() {
      return m_strClientName;
   };
   //! Returns the main MIDAS handle to the ODB
   HNDLE GetHdb() {
      return m_hDB;
   };
   //! Returns if already connected to an experiment
   BOOL IsConnected() {
      return m_bConnected;
   };
   BOOL StartTimer();
   BOOL Connect(QString host, QString experiment, QString client);
   void Disconnect();

   // slots
   private slots:void timeOutProc();
};

#endif                          // _CMEXPERIMENT_H
