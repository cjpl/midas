/********************************************************************************************

  cmExperiment.h

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/25
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

#ifndef _CMEXPERIMENT_H
#define _CMEXPERIMENT_H

#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>

#include "midas.h"

void GetPassword(char *password);

class cmExperiment : public QObject
{
  Q_OBJECT

  // Construction
  public:
    cmExperiment(QString strClientName, QObject *parent = 0, const char *name = 0);
    ~cmExperiment();

  // Attributes
  private:
    QString m_strHostName;
    QString m_strExperimentName;
    QString m_strClientName;
    BOOL    m_bConnected;
    BOOL    m_bTimerStarted;
    HNDLE   m_hDB;
    QTimer  *timer;

  // Operations
  public:
    void    SetHostName(QString hostName) { m_strHostName = hostName; };
    void    SetExperimentName(QString expName) { m_strExperimentName = expName; };
    QString GetHostName()        { return m_strHostName; };
    QString GetExperimentName()  { return m_strExperimentName; };
    QString GetClientName()      { return m_strClientName; };
    HNDLE   GetHdb()             { return m_hDB; };
    BOOL    IsConnected()        { return m_bConnected; };
    BOOL    StartTimer();
    BOOL    Connect(QString host, QString experiment, QString client);
    void    Disconnect();

  // slots
  private slots:
    void    timeOutProc();
};

#endif // _CMEXPERIMENT_H
