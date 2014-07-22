/********************************************************************************************

   PHvEdit.h

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

#ifndef _PHVEDIT_H_
#define _PHVEDIT_H_

// definitions --------------------------------
#define FE_TIMEOUT 60

#define HV_NAME          0
#define HV_DEMAND        1
#define HV_MEASURED      2
#define HV_CURRENT       3

#define CL_NAME          0
#define CL_CURRENT       1
#define CL_CURRENT_LIMIT 2
// ---------------------------------------------

#include <qstatusbar.h>
#include <qstring.h>
#include <qxml.h>
#include <qptrlist.h>
#include <qtimer.h>

#include "PHvAdmin.h"
#include "PExperiment.h"
#include "PKey.h"

#include "PHvEditBase.h"

/*!
 * <p>Helper structure needed to handle hv_setting files
 */
typedef struct {
  QString name;         //!< HV channel name
  int     ch;           //!< HV channel number
  float   demandHV;     //!< HV demand value in (V) or (kV) depending in the demandInV flag
  float   currentLimit; //!< HV current limit (mA)
} hvLoad;


class PHvEdit;

void NamesChanged(int, int, void *info);
void HVChanged(HNDLE, HNDLE, void *info);

//-----------------------------------------------------------------------------
class PHvEditXMLParser : public QXmlDefaultHandler
{
  public:
    enum EHvEditKeyWords {empty, hvDeviceName, hvCh, chNo, name, hvDemand, currentLimit};

    PHvEditXMLParser(PHvEdit*);

    bool startDocument();
    bool startElement( const QString&, const QString&, const QString& ,
                       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters(const QString&);
    bool endDocument();

  private:
    PHvEdit        *fHve;       //!< pointer to the calling class
    EHvEditKeyWords fKey;       //!< key tags for the parsing process
    int             fChCounter; //!< HV channel counter
};

//-----------------------------------------------------------------------------
class PHvEdit : public PHvEditBase
{
  Q_OBJECT

  public:
    PHvEdit(PHvAdmin *hvA, PExperiment *cmExp,
            QWidget *parent = 0, const char *name = 0, WFlags fl = WType_TopLevel);
    ~PHvEdit();

  protected slots:
    void FileOpen();
    void FileSave();
    void FilePrint();
    void FileExit();
    void HelpIndex();
    void HelpContents();
    void HelpAboutQt();
    void HelpAbout();
    void ChangedDeviceSelection();
    void OnSelectAll();
    void OnSwitchAllChannelsOff();
    void OnZero();
    void OnRestore();
    void OnM100();
    void OnM010();
    void OnM001();
    void OnP100();
    void OnP010();
    void OnP001();
    void OnSet();
    void OnScale();
    void HvValueChanged(int row, int col);
    void CurrentLimitValueChanged(int row, int col);
    void DisconnectAndQuit();
    void NetworkProblems();

  // Attributes
  private:
    PHvAdmin       *fHvA;             //!< pointer to the 'hvEdit' administration object
    PExperiment    *fExp;             //!< pointer to the MIDAS experiment object
    QPtrList<PKey> *fRootKey;         //!< pointer list to the various HV ODB root keys
    QPtrList<PKey> *fNamesKey;        //!< pointer list to the various HV ODB name keys
    QPtrList<PKey> *fDemandKey;       //!< pointer list to the various HV ODB demand value keys
    QPtrList<PKey> *fMeasuredKey;     //!< pointer list to the various HV ODB measured value keys
    QPtrList<PKey> *fCurrentKey;      //!< pointer list to the various HV ODB measured current keys
    QPtrList<PKey> *fCurrentLimitKey; //!< pointer list to the various HV ODB current limit keys

    INT     f_nChannels;         //!< number of overall HV channels
    INT     f_nNameLength;       //!< length of the HV channel name labels
    INT     f_nGroups;           //!< number of different HV groups, e.g. TD%MCP3: here TD is the group
    INT     f_iGroup;            //!<
    INT     f_iGroupCache;       //!<
    char   *fName;               //!< c like array of all the channel names,
                                 //!  i.e. size of m_Name = m_nChannels * m_nNameLength
    float  *fDemand;             //!< c like array to store all HV demand values
                                 //!  (number of elements = m_nChannels)
    float  *fMeasured;           //!< c like array to store all HV measured values
                                 //!  (number of elements = m_nChannels)
    float  *fRestore;            //!< c like array to store all HV previous demand values
                                 //!  (number of elements = m_nChannels)
    float  *fCurrent;            //!< c like array to store all HV current values
                                 //!  (number of elements = m_nChannels)
    float  *fCurrentLimit;       //!< c like array to store all HV current limit values
                                 //!  (number of elements = m_nChannels)
    float  *fDemandCache;        //!< cache for demand HV values. Used to check if values changed
                                 //!  since last reading. Used to have an efficient updating scheme
    float  *fMeasuredCache;      //!< cache for measured HV values. Details see m_DemandCache
    float  *fCurrentCache;       //!< cache for measured current values. Details see m_DemandCache
    float  *fCurrentLimitCache;  //!< cache for current limit values. Details see m_DemandCache
    INT    *fSelection;          //!<
    INT    *fGroup;              //!< group tag array of length m_nChannels. Mapping from channel to
                                 //!  HV group. The structure is e.g. {0,0,0,0,1,1,2,2,..,2,3,..,3,..}
                                 //!  where the first 4 elements (channels) are belonging to HV group 0 etc.
    INT    *fOdbRootMap;         //!< root map tag array of length m_nChannels. Mapping from channel to
                                 //!  ODB HV root. The structure is e.g. {0,0,0,0,1,1,2,2,..,2,3,..,3,..}
                                 //!  where the first 4 elements (channels) are belonging to ODB HV
                                 //!  root 0 etc.

    QString fDevName;            //!< device name found in the HV load file
    int     fNoOfCh;             //!< number of channels found in the HV load file
    hvLoad  fHvLoadData[256];    //!< HV channel information found in the HV load file
    
    bool    fDemandInV;          //!< flag indicating if the demand HV is given in V (true) or kV (false)

    QTimer  *fTerminationTimer;  //!< timer which terminates hvEdit it there is for too long no user interaction

  // Operations
  private:
    void SetHostName(QString host);
    void SetExpName(QString experiment);
    BOOL UpdateChannelDefinitions();
    void UpdateTable(int ch, bool forced_update);
    int  ConnectToExp();
    void DisconnectFromExp();
    void EnableConnected();
    BOOL OpenKeys();
    void CloseKeys();
    void FreeMem();
    int  ChannelIndex(int ch);
    void ClearTable();
    void UpdateOdbDemand(int ch);
    void UpdateOdbCurrentLimit(int ch);
    void Increment(const float incr);
    void ScaleHv(const float scale);
    void RestartTimeoutTimer();

    friend class PHvEditXMLParser;

    friend void NamesChanged(int, int, void*);
    friend void HVChanged(HNDLE, HNDLE, void *info);
};

#endif // _PHVEDIT_H_


