/********************************************************************************************

   Qt_hvEdit.h

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/21
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.2  2003/10/21 14:39:06  suter_a
  added scaling

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

#ifndef QT_HVEDIT_H
#define QT_HVEDIT_H

// definitions --------------------------------
#define FE_TIMEOUT 90

#define HV_NAME          0
#define HV_DEMAND        1
#define HV_MEASURED      2
#define HV_CURRENT       3

#define CL_NAME          0
#define CL_CURRENT       1
#define CL_CURRENT_LIMIT 2
// ---------------------------------------------

#include <qstatusbar.h>

#include "hvAdmin.h"
#include "cmExperiment.h"
#include "cmKey.h"

#include "Qt_hvEdit_Base.h"

class Qt_hvEdit : public Qt_hvEdit_Base
{
  Q_OBJECT

  public:
    Qt_hvEdit(hvAdmin *hvA, cmExperiment *cmExp,
              QWidget *parent = 0, const char *name = 0, WFlags fl = WType_TopLevel);
    ~Qt_hvEdit();

  public:
    void setHostName(QString host);
    void setExpName(QString experiment);
    BOOL updateChannelDefinitions();
    void updateTable(int ch);

  public slots:
    void connectToExp();
    void disconnectFromExp();

  protected slots:
    void fileOpen();
    void fileSave();
    void filePrint();
    void fileExit();
    void helpIndex();
    void helpContents();
    void helpAboutQt();
    void helpAbout();
    void changedDeviceSelection();
    void onSelectAll();
    void onSwitchAllChannelsOff();
    void onZero();
    void onRestore();
    void onM100();
    void onM010();
    void onM001();
    void onP100();
    void onP010();
    void onP001();
    void onSet();
    void onScale();
    void hvValueChanged(int row, int col);
    void currentLimitValueChanged(int row, int col);

  // Attributes
  private:
    hvAdmin      *hvA;
    cmExperiment *cmExp;
    cmKey        *rootKey;
    cmKey        *namesKey;
    cmKey        *demandKey;
    cmKey        *measuredKey;
    cmKey        *currentKey;
    cmKey        *currentLimitKey;

    INT    m_nChannels, m_nNameLength,  m_nGroups, m_iGroup, m_iGroupCache;
    char  *m_Name;
    float *m_Demand, *m_Measured, *m_Restore, *m_Current, *m_CurrentLimit;
    float *m_DemandCache, *m_MeasuredCache, *m_CurrentCache, *m_CurrentLimitCache;
    INT   *m_Selection, *m_Group;

  // Operations
  private:
    void disableNotConnected();
    void enableConnected();
    BOOL openKeys();
    void closeKeys();
    void freeMem();
    int  channelIndex(int ch);
    void clearTable();
    void updateODB_demand(int ch);
    void updateODB_currentLimit(int ch);
    void increment(const float incr);
    void scale_hv(const float scale);
    QString *findSub(char *str, char *first, char *last, QString *last);
};

void namesChanged(int,int,void*);
void HVChanged(HNDLE hDB, HNDLE hKey, void *info);

#endif // QT_HVEDIT_H


