/********************************************************************************************

  PHvEdit.cpp

  implementation of the virtual class 'PHvEditBase' from the Qt designer
  (see 'PHvEditBase.ui')

  Main GUI which handles the interaction between a user and the midas ODB.

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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <qapplication.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfile.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qtable.h>
#include <qtabwidget.h>
#include <qcombobox.h>
#include <qdialog.h>
#include <qaction.h>
#include <qpushbutton.h>
#include <qdatetime.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qtooltip.h>

#include "midas.h"

#include "PFileList.h"
#include "PHvEditAbout.h"
#include "PHvEdit.h"

#include "help/PHelpWindow.h"

#include "PGlobals.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// implementation of PHvEditXMLParser
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//**********************************************************************
// constructor
//**********************************************************************
/*!
 * <p>XML Parser class for the high voltage setting files.
 *
 * \param hve pointer to the main class.
 */
PHvEditXMLParser::PHvEditXMLParser(PHvEdit *hve)
{
  fHve = hve;
  fKey = empty;
  fChCounter = -1;
}

//**********************************************************************
// startDocument
//**********************************************************************
/*!
 * <p>Routine called at the beginning of the XML parsing process.
 */
bool PHvEditXMLParser::startDocument()
{
  return TRUE;
}

//**********************************************************************
// startElement
//**********************************************************************
/*!
 * <p>Routine called when a new XML tag is found. Here it is used
 * to set a tag for filtering afterwards the content.
 *
 * \param qName name of the XML tag.
 */
bool PHvEditXMLParser::startElement( const QString&, const QString&,
                                       const QString& qName,
                                       const QXmlAttributes& )
{
  if (qName == "hvDeviceName") {
    fKey = hvDeviceName;
  } else if (qName == "hvCh") {
    fKey = hvCh;
  } else if (qName == "chNo") {
    fKey = chNo;
  } else if (qName == "name") {
    fKey = name;
  } else if (qName == "hvDemand") {
    fKey = hvDemand;
  } else if (qName == "currentLimit") {
    fKey = currentLimit;
  }

  return TRUE;
}

//**********************************************************************
// endElement
//**********************************************************************
/*!
 * <p>Routine called when the end XML tag is found. It is used to
 * put the filtering tag to 'empty'.
 */
bool PHvEditXMLParser::endElement( const QString&, const QString&, const QString& )
{
  fKey = empty;

  return TRUE;
}

//**********************************************************************
// characters
//**********************************************************************
/*!
 * <p>This routine delivers the content of an XML tag. It fills the
 * content into the load data structure.
 */
bool PHvEditXMLParser::characters(const QString& str)
{
  switch (fKey) {
    case hvDeviceName:
      fHve->fDevName = QString(str.ascii()).stripWhiteSpace();
      break;
    case hvCh:
      fChCounter++;
      break;
    case chNo:
      fHve->fHvLoadData[fChCounter].ch = str.toInt();
      break;
    case name:
      fHve->fHvLoadData[fChCounter].name = str;
      break;
    case hvDemand:
      fHve->fHvLoadData[fChCounter].demandHV = str.toFloat();
      break;
    case currentLimit:
      fHve->fHvLoadData[fChCounter].currentLimit = str.toFloat();
      break;
    default:
      break;
  }

  return TRUE;
}

//**********************************************************************
// endDocument
//**********************************************************************
/*!
 * <p>Called at the end of the XML parse process.
 */
bool PHvEditXMLParser::endDocument()
{
  fHve->fNoOfCh = fChCounter+1;
  return TRUE;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// implementation of PHvEdit
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//*******************************************************************************************************
//  Constructor
//*******************************************************************************************************
/*!
 * <p>Initializes the widget and readjust some of the GUI's parameters which cannot be done in
 * the designer directly. It furthermore connects all the necessary signal with the corresponding
 * slots.
 * 
 * \param hvA    pointer to the administration object which holds
 * \param cmExp  pointer to the experiment object
 * \param parent pointer to the parent widget
 * \param name   is some text that can be used to identify the object, e.g. in the designer.
 * \param fl     widget flags
 */
PHvEdit::PHvEdit(PHvAdmin *hvA, PExperiment *cmExp, QWidget *parent, const char *name,
          WFlags fl) : PHvEditBase(parent, name, fl)
{
  fHvA = hvA;
  fExp = cmExp;
  fExp->SetHostName(*fHvA->GetHostName());
  fExp->SetExperimentName(*fHvA->GetExpName());

  // initialize all working pointers to NULL
  fName = NULL;
  fDemand = NULL;
  fMeasured = NULL;
  fRestore = NULL;
  fCurrent = NULL;
  fCurrentLimit = NULL;
  fDemandCache = NULL;
  fMeasuredCache = NULL;
  fCurrentCache = NULL;
  fCurrentLimitCache = NULL;
  fSelection = NULL;
  fGroup = NULL;
  fOdbRootMap = NULL;

  // initialize keys properly
  fRootKey = new QPtrList<PKey>();
  fRootKey->setAutoDelete(true);
  fNamesKey = new QPtrList<PKey>();
  fNamesKey->setAutoDelete(true);
  fDemandKey = new QPtrList<PKey>();
  fDemandKey->setAutoDelete(true);
  fMeasuredKey = new QPtrList<PKey>();
  fMeasuredKey->setAutoDelete(true);
  fCurrentKey = new QPtrList<PKey>();
  fCurrentKey->setAutoDelete(true);
  fCurrentLimitKey = new QPtrList<PKey>();
  fCurrentLimitKey->setAutoDelete(true);

  // initialize table cells of the GUI
  fHv_table->setColumnReadOnly(HV_NAME, TRUE);
  fHv_table->setColumnReadOnly(HV_MEASURED, TRUE);
  fHv_table->setColumnReadOnly(HV_CURRENT, TRUE);
  fCurrentLimit_table->setColumnReadOnly(CL_NAME, TRUE);
  fCurrentLimit_table->setColumnReadOnly(CL_CURRENT, TRUE);

  // set column width of the table cells
  fHv_table->setColumnWidth(HV_NAME, 80);
  fHv_table->setColumnWidth(HV_DEMAND, 65);
  fHv_table->setColumnWidth(HV_MEASURED, 65);
  fHv_table->setColumnWidth(HV_CURRENT, 60);
  fCurrentLimit_table->setColumnWidth(CL_NAME, 80);
  fCurrentLimit_table->setColumnWidth(CL_CURRENT, 60);
  fCurrentLimit_table->setColumnWidth(CL_CURRENT_LIMIT, 85);
  
  // init flag which is indicating if the demand HV is in 'V' (true) or 'kV' (false) 
  fDemandInV = fHvA->GetDemandInV();
  
  // set increment/decrement labels properly
  if (fDemandInV) { // everything in V
    fM100_pushButton->setText("-100");
    fM010_pushButton->setText("-10");
    fM001_pushButton->setText("-1");
    fP100_pushButton->setText("+100");
    fP010_pushButton->setText("+10");
    fP001_pushButton->setText("+1");
  } else { // everything in kV
    fM100_pushButton->setText("-1.0");
    fM010_pushButton->setText("-0.1");
    fM001_pushButton->setText("-0.01");
    fP100_pushButton->setText("+1.0");
    fP010_pushButton->setText("+0.1");
    fP001_pushButton->setText("+0.01");
  }
  // set the tool tip properly
  if (fDemandInV) { // everything in V
    QToolTip::remove(fM100_pushButton);
    QToolTip::add(fM100_pushButton, "decrease the selected HV by 100 (V)");
    QToolTip::remove(fM010_pushButton);
    QToolTip::add(fM010_pushButton, "decrease the selected HV by 10 (V)");
    QToolTip::remove(fM001_pushButton);
    QToolTip::add(fM001_pushButton, "decrease the selected HV by 1 (V)");
    QToolTip::remove(fP100_pushButton);
    QToolTip::add(fP100_pushButton, "increase the selected HV by 100 (V)");
    QToolTip::remove(fP010_pushButton);
    QToolTip::add(fP010_pushButton, "increase the selected HV by 10 (V)");
    QToolTip::remove(fP001_pushButton);
    QToolTip::add(fP001_pushButton, "increase the selected HV by 1 (V)");
  } else { // everything in kV
    QToolTip::remove(fM100_pushButton);
    QToolTip::add(fM100_pushButton, "decrease the selected HV by 1.0 (kV)");
    QToolTip::remove(fM010_pushButton);
    QToolTip::add(fM010_pushButton, "decrease the selected HV by 0.1 (kV)");
    QToolTip::remove(fM001_pushButton);
    QToolTip::add(fM001_pushButton, "decrease the selected HV by 0.01 (kV)");
    QToolTip::remove(fP100_pushButton);
    QToolTip::add(fP100_pushButton, "increase the selected HV by 1.0 (kV)");
    QToolTip::remove(fP010_pushButton);
    QToolTip::add(fP010_pushButton, "increase the selected HV by 0.1 (kV)");
    QToolTip::remove(fP001_pushButton);
    QToolTip::add(fP001_pushButton, "increase the selected HV by 0.01 (kV)");
  }

  fDevName = QString("");  

  // connect signals with slots
  connect(fHv_table, SIGNAL(valueChanged(int, int)), SLOT(HvValueChanged(int, int)));
  connect(fCurrentLimit_table, SIGNAL(valueChanged(int, int)), SLOT(CurrentLimitValueChanged(int, int)));
  connect(fExp, SIGNAL(ReceivedShutdownCmd()), this, SLOT(DisconnectAndQuit()));
  connect(fExp, SIGNAL(NetworkConnectionLost()), this, SLOT(NetworkProblems()));

  // setup termination timeout handler
  if (fHvA->GetTerminationTimeout() > 0) { // only start termination timeout handler if timeout > 0
    fTerminationTimer = new QTimer();
    fTerminationTimer->start(1000*60*fHvA->GetTerminationTimeout()); // start timeout timer
    connect(fTerminationTimer, SIGNAL(timeout()), this, SLOT(DisconnectAndQuit()));
  }
  
  // connect to the experiment
  if (!ConnectToExp())
    exit(0);
}

//*******************************************************************************************************
//  Destructor
//*******************************************************************************************************
/*!
 * <p>Destructor. Disconnects from the MIDAS experiment.
 */
PHvEdit::~PHvEdit()
{
  DisconnectFromExp();
}

//*******************************************************************************************************
//  implementation of SetHostName
//*******************************************************************************************************
/*!
 * <p>Sets the host name in the widget
 *
 * \param host name
 */
void PHvEdit::SetHostName(QString host)
{
  fHost_lineEdit->setText(host);
}

//*******************************************************************************************************
//  implementation of SetExpName
//*******************************************************************************************************
/*!
 * <p>Sets the experiment name in the widget
 *
 * \param experiment name
 */
void PHvEdit::SetExpName(QString experiment)
{
  fExp_lineEdit->setText(experiment);
}

//*******************************************************************************************************
//  implementation of UpdateChannelDefinitions
//*******************************************************************************************************
/*!
 * <p>It mainly allocates all the necessary memory for the HV channels and establishes the necessary
 * hotlinks.
 */
BOOL PHvEdit::UpdateChannelDefinitions()
{
  int  i, j, k;
  int  offset;
  char str[80];

  QListBoxItem *lBI;

  // remove hot links ----------------------------------------------------------------
  for (i=0; i<fHvA->fNoOdbRoots; i++) { // loop over all ODB HV roots
    fNamesKey->at(i)->Unlink();
    fDemandKey->at(i)->Unlink();
    fMeasuredKey->at(i)->Unlink();
    fCurrentKey->at(i)->Unlink();
    fCurrentLimitKey->at(i)->Unlink();
  }

  // update keys ---------------------------------------------------------------------
  for (i=0; i<fHvA->fNoOdbRoots; i++) { // loop over all ODB HV roots
    fNamesKey->at(i)->UpdateKey();
    fDemandKey->at(i)->UpdateKey();
    fMeasuredKey->at(i)->UpdateKey();
    fCurrentKey->at(i)->UpdateKey();
    fCurrentLimitKey->at(i)->UpdateKey();
  }

  // free used memory
  FreeMem();

  // initialize variables ------------------------------------------------------------
  // extract number of HV channels
  f_nChannels = 0;
  for (i=0; i<fHvA->fNoOdbRoots; i++) // loop over all ODB HV roots
    f_nChannels += fNamesKey->at(i)->GetNumValues();
  f_nNameLength = fNamesKey->at(0)->GetItemSize();
  f_nGroups = 0;
  f_iGroupCache = -1;
  fName = (char *) malloc(f_nChannels * f_nNameLength * sizeof(char));
  fSelection = (INT *) malloc(f_nChannels * sizeof(INT));
  fGroup = (INT *) calloc(f_nChannels, sizeof(INT));
  fOdbRootMap = (INT *) calloc(f_nChannels, sizeof(INT));
  fRestore = (float*) calloc(f_nChannels, sizeof(float));
  fDemand = (float *) calloc(f_nChannels, sizeof(float));
  fDemandCache = (float *) calloc(f_nChannels, sizeof(float));
  fMeasured = (float *) calloc(f_nChannels, sizeof(float));
  fMeasuredCache = (float *) calloc(f_nChannels, sizeof(float));
  fCurrent = (float *) calloc(f_nChannels, sizeof(float));
  fCurrentCache = (float *) calloc(f_nChannels, sizeof(float));
  fCurrentLimit = (float *) calloc(f_nChannels, sizeof(float));
  fCurrentLimitCache = (float *) calloc(f_nChannels, sizeof(float));

  // Link names entry to m_Name array
  offset=0;
  for (i=0; i<fHvA->fNoOdbRoots; i++) { // loop over all ODB HV roots
    fNamesKey->at(i)->HotLink((void *) &fName[offset*f_nNameLength], MODE_READ, NamesChanged, this);
    offset += fNamesKey->at(i)->GetNumValues();
  }

  // reset group list
  fDevice_listBox->clear();

  // fill ODB root map tag array
  k=0;
  for (i=0; i<fHvA->fNoOdbRoots; i++)
    for (j=0; j<fNamesKey->at(i)->GetNumValues(); j++)
      fOdbRootMap[k++] = i;

  // Analyze names, generate groups
  for (i=0; i<f_nChannels; i++) {
    if (strchr((const char *) fName+i*f_nNameLength, '%')) {
      strcpy(str, (const char *) fName+i*f_nNameLength);
      *const_cast<char*>(strchr((const char *) str, '%')) = 0;
      if ((lBI = fDevice_listBox->findItem(str, Qt::ExactMatch))==0) { // device item new
        fDevice_listBox->insertItem(str);
        fGroup[i] = f_nGroups;
        f_nGroups++;
      }
      else
        fGroup[i] = fDevice_listBox->index(lBI); // get index of already existing item
    }
  }
  fDevice_listBox->setSelected(0, TRUE); // select the first item in the list
  f_iGroup = 0;

  // Link demand entry to fDemand array
  offset=0;
  for (i=0; i<fHvA->fNoOdbRoots; i++) { // loop over all ODB HV roots
    if (fDemandKey->at(i)->GetType() != TID_FLOAT) {
      MidasMessage("Wrong 'demand' entry in HV values");
      return FALSE;
    }
    fDemandKey->at(i)->HotLink((void *) &fDemand[offset], MODE_READ, HVChanged, this);
    offset += fDemandKey->at(i)->GetNumValues();
  }

  // Link measured entry to fMeasured array
  offset=0;
  for (i=0; i<fHvA->fNoOdbRoots; i++) { // loop over all ODB HV roots
    if (fMeasuredKey->at(i)->GetType() != TID_FLOAT) {
      MidasMessage("Wrong 'measured' entry in HV values");
      return FALSE;
    }
    fMeasuredKey->at(i)->HotLink((void *) &fMeasured[offset], MODE_READ, HVChanged, this);
    offset += fMeasuredKey->at(i)->GetNumValues();
  }

  // Link current entry to fCurrent array
  offset=0;
  for (i=0; i<fHvA->fNoOdbRoots; i++) { // loop over all ODB HV roots
    if (fCurrentKey->at(i)->GetType() != TID_FLOAT) {
      MidasMessage("Wrong 'current' entry in HV values");
      return FALSE;
    }
    fCurrentKey->at(i)->HotLink((void *) &fCurrent[offset], MODE_READ, HVChanged, this);
    offset += fCurrentKey->at(i)->GetNumValues();
  }

  // Link current limit entry to fCurrentLimit array
  offset=0;
  for (i=0; i<fHvA->fNoOdbRoots; i++) { // loop over all ODB HV roots
    if (fCurrentLimitKey->at(i)->GetType() != TID_FLOAT) {
      MidasMessage("Wrong 'current limit' entry in HV values");
      return FALSE;
    }
    fCurrentLimitKey->at(i)->HotLink((void *) &fCurrentLimit[offset], MODE_READ, HVChanged, this);
    offset += fCurrentLimitKey->at(i)->GetNumValues();
  }

  return TRUE;
}

//*******************************************************************************************************
//  implementation of UpdateTable
//*******************************************************************************************************
/*!
 * <p>Updates the table of the HV's and the currents.
 *
 * \param ch channel number, if == -1 all channels are meant
 * \param forced_update flag, if true the whole table is updated and not just the necessary parts
 */
void PHvEdit::UpdateTable(int ch, bool forced_update)
{
  int   i, offset;
  char  name[256], demand[256], measured[256], current[256], current_limit[256];
  bool  need_update, device_changed;
  float hv_tol;
  int   prec;

  if (ch>=0) { // channel number given
    // get correct index
    i = ChannelIndex(ch);
    // get name from ODB
    offset = fOdbRootMap[ch];
    db_sprintf(name, fName, fNamesKey->at(offset)->GetItemSize(), i, TID_STRING);
    // filter out channel name
    if (strchr(name, '%'))
      strcpy(name, strchr(name, '%')+1);
    // overwrite name cell
    fHv_table->setText(ch, HV_NAME, name);
    fCurrentLimit_table->setText(ch, CL_NAME, name);

    // get demand HV from ODB
    db_sprintf(demand, fDemand, sizeof(float), i, TID_FLOAT);
    // overwrite demand cell
    fHv_table->setText(ch, HV_DEMAND, demand);

    // get measured HV from ODB
    db_sprintf(measured, fMeasured, sizeof(float), i, TID_FLOAT);
    // overwrite measured cell
    fHv_table->setText(ch, HV_MEASURED, measured);

    // get measured current from ODB
    db_sprintf(current, fCurrent, sizeof(float), i, TID_FLOAT);
    // overwrite current cell
    fHv_table->setText(ch, HV_CURRENT, current);
    fCurrentLimit_table->setText(ch, CL_CURRENT, current);

    // get measured current from ODB
    db_sprintf(current_limit, fCurrentLimit, sizeof(float), i, TID_FLOAT);
    // overwrite current cell
    fCurrentLimit_table->setText(ch, CL_CURRENT_LIMIT, current_limit);

    // Update cache
    fDemandCache[i] = fDemand[i];
    fMeasuredCache[i] = fMeasured[i];

  } else { // check all channels
    // check if update is needed
    need_update = FALSE;
    device_changed = FALSE;

    // tolerance depends on the demand HV units
    if (fDemandInV) {
      hv_tol = 0.01;   // 0.01 V since V units
      prec   = 2;      // 2 significant digits
    } else {
      hv_tol = 0.0001; // 0.1V since kV units
      prec   = 3;      // 3 significant digits
    }
    
    if (f_iGroup == f_iGroupCache) {
      for (i=0 ; i<f_nChannels ; i++) {
        if (fGroup[i] == f_iGroup &&
            ((fabs(fDemand[i]-fDemandCache[i])>hv_tol) ||
             (fabs(fMeasured[i]-fMeasuredCache[i])>hv_tol) ||
             (fabs(fCurrent[i]-fCurrentCache[i])>0.001) ||
             (fabs(fCurrentLimit[i]-fCurrentLimitCache[i])>0.001))) {
          need_update = TRUE;
          break;
        }
      }
      if (!need_update)
        return;
    } else {
      device_changed = TRUE;
      need_update = TRUE;
    }

    f_iGroupCache = f_iGroup;

    if (device_changed || forced_update)
      ClearTable(); // remove all items from the table

    offset = 0; // to get offset between devices
    for (i=0 ; i<f_nChannels ; i++) {
      if (fGroup[i] != f_iGroup) {
        offset++;
        continue;
      }
      if (device_changed || forced_update) {
        // get ch name
        db_sprintf(name, fName, fNamesKey->at(0)->GetItemSize(), i, TID_STRING);
        // filter out ch name
        if (strchr(name, '%'))
          strcpy(name, strchr(name, '%')+1);
        // check if enough rows are available
        if (i-offset>=fHv_table->numRows()) {
          // add necessary rows
          fHv_table->insertRows(fHv_table->numRows());
          fCurrentLimit_table->insertRows(fCurrentLimit_table->numRows());
        }
        // overwrite table cells
        fHv_table->setItem(i-offset, HV_NAME, new QTableItem(fHv_table, QTableItem::Never, name));
        fCurrentLimit_table->setItem(i-offset, HV_NAME, new QTableItem(fHv_table, QTableItem::Never, name));
      }

      if ((fabs(fDemand[i]-fDemandCache[i])>hv_tol) || forced_update) {
        // overwrite demand cell
        fHv_table->setText(i-offset, HV_DEMAND, QString("%1").arg(fDemand[i],0,'f',prec));
        fHv_table->updateCell(i-offset, HV_DEMAND);
      }

      if ((fabs(fMeasured[i]-fMeasuredCache[i])>hv_tol) || forced_update) {
        // overwrite measured cell
        fHv_table->setText(i-offset, HV_MEASURED, QString("%1").arg(fMeasured[i],0,'f',prec));
        fHv_table->updateCell(i-offset, HV_MEASURED);
      }

      if ((fabs(fCurrent[i]-fCurrentCache[i])>hv_tol) || forced_update) {
        // overwrite current cell
        fHv_table->setText(i-offset, HV_CURRENT, QString("%1").arg(fCurrent[i],0,'f',4));
        fHv_table->updateCell(i-offset, HV_CURRENT);
        fCurrentLimit_table->setText(i-offset, CL_CURRENT, QString("%1").arg(fCurrent[i],0,'f',4));
        fCurrentLimit_table->updateCell(i-offset, CL_CURRENT);
      }

      if ((fabs(fCurrentLimit[i]-fCurrentLimitCache[i])>hv_tol) || forced_update) {
        // overwrite current cell
        fCurrentLimit_table->setText(i-offset, CL_CURRENT_LIMIT, QString("%1").arg(fCurrentLimit[i],0,'f',4));
        fCurrentLimit_table->updateCell(i-offset, CL_CURRENT_LIMIT);
      }

      // Update cache
      fDemandCache[i] = fDemand[i];
      fMeasuredCache[i] = fMeasured[i];
    }
  }

  // display warning message if FE not running
  for (i=0; i<fHvA->fNoOdbRoots; i++) {
    if (fMeasuredKey->at(i)->GetKeyTime() > FE_TIMEOUT)
      statusBar()->message(QString("WARNING: FE #%1 not running").arg(i), 2000);
  }
}

//*******************************************************************************************************
//  implementation of FileOpen
//*******************************************************************************************************
/*!
 * <p>Reads a HV settings file from the disc and, if valid, sets the values.
 */
void PHvEdit::FileOpen()
{
  int i, j, result, offset;
  QString qname;

  QString fln = QFileDialog::getOpenFileName(*fHvA->GetHvDefaultDir(),
                    "high voltage settings (*.xml)",
                    this,
                    "open file dialog"
                    "Choose a file" );

  // if no file name chosen, i.e. Cancel was selected
  if (fln.isEmpty()) {
    return;
  }

  // XML Parser part
  PHvEditXMLParser handler(this);
  QFile xmlFile(fln);
  QXmlInputSource source( &xmlFile );
  QXmlSimpleReader reader;
  reader.setContentHandler( &handler );
  reader.parse( source );

  PFileList *hvList = new PFileList();
  
  // set header properly according to the demand HV units
  if (fDemandInV)
    hvList->fHvTable->horizontalHeader()->setLabel(2, tr("HV Demand (V)"));
  else
    hvList->fHvTable->horizontalHeader()->setLabel(2, tr("HV Demand (kV)"));
  
  // check if table is large enough
  if (hvList->fHvTable->numRows()<fNoOfCh) { // add rows
    hvList->fHvTable->insertRows(hvList->fHvTable->numRows(), fNoOfCh-hvList->fHvTable->numRows());
  }
  // generate a file list
  for (j=0; j<fNoOfCh; j++) { // row
    hvList->fHvTable->setText(j,0,fHvLoadData[j].name);
    hvList->fHvTable->setText(j,1,QString("%1").arg(fHvLoadData[j].ch));
    hvList->fHvTable->setText(j,2,QString("%1").arg(fHvLoadData[j].demandHV));
    hvList->fHvTable->setText(j,3,QString("%1").arg(fHvLoadData[j].currentLimit));
  }

  if (!hvList->exec()) { // do not load
    QMessageBox::information(0, "INFO", "will NOT load data.", QMessageBox::Ok);
    if (hvList) {
      delete hvList;
      hvList=0;
    }
    return;
  }

  // check if device is OK and get ODB device offset
  offset = 0;
  for (i=0; i<f_nChannels; i++) { // parse for device name
    qname = fName+i*f_nNameLength;
    if (qname.find(fDevName,0,FALSE)!=-1) { // found
      break;
    }
    offset++;
  }
  if (i==f_nChannels) { // device name couldn't be found
    QMessageBox::warning(0, "ERROR", "device name "+qname+" couldn't be found!",
                         QMessageBox::Ok, QMessageBox::NoButton);
    if (hvList) {
      delete hvList;
      hvList=0;
    }
    return;
  }

  // check if names are OK
  for (i=0; i<fNoOfCh; i++) { // compare ch names
    j = offset+fHvLoadData[i].ch-1; // get the right address
    qname = fName+j*f_nNameLength;
    if (qname.find(fHvLoadData[i].name,0,FALSE)==-1) { // ch name load != ch name ODB
      result = QMessageBox::warning(0, "WARNING", "ch name "+fHvLoadData[i].name+
           " different from ODB ch name.\nThough, it is NOT going to be updated!!\nThis Message only appears once.",
               QMessageBox::Ok, QMessageBox::NoButton);
      break;
    }
  }

  // set values
  for (i=0; i<fNoOfCh; i++) { // update demand and current limit values
    j = offset+fHvLoadData[i].ch-1; // get right address
    fRestore[j] = fDemand[j];
    fDemand[j] = fHvLoadData[i].demandHV;
    fCurrentLimit[j] = fHvLoadData[i].currentLimit;
  }

  // update ODB and GUI
  UpdateOdbDemand(-1);
  UpdateOdbCurrentLimit(-1);
  UpdateTable(-1, true);

  if (hvList) {
    delete hvList;
    hvList=0;
  }

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of FileSave
//*******************************************************************************************************
/*!
 * <p>Extracts the channel names, demand HV's, and current limits and saves them in a XML file, which
 * can be reloaded at any time.
 */
void PHvEdit::FileSave()
{
  int     i, offset;
  QString str, device, fln;
  char    *sub;
  FILE    *fp;

  // get active device group
  device = fDevice_listBox->currentText();

  // open save as dialog
  fln = QFileDialog::getSaveFileName(*(fHvA->GetHvDefaultDir()), "HV Settings (*.xml)", this,
                                     "Save HV Settings...");
  if (fln.isEmpty()) { // empty file name
    return ;
  }
  // check if file exists
  QFileInfo flnInfo(fln);
  if (flnInfo.size() > 0) { // file exists
    i = QMessageBox::warning(0, "WARNING", "Do you want to overwrite this file?", QMessageBox::Yes, QMessageBox::No);
    if (i == QMessageBox::No)
      return;
  }

  // open file
  if ((fp=fopen(fln, "w"))==NULL) { // couldn't open file
    QMessageBox::warning(0, "ERROR", "Couldn't open file for saving!!", QMessageBox::Ok, QMessageBox::NoButton);
    return;
  }

  // write header
  fputs("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n", fp);
  fputs("<HV xmlns=\"http://midas.web.psi.ch/HV\">\n", fp);
  // write comment
  fputs("<comment>\n", fp);
  if (fDemandInV)
    fputs("chHV in (V),\n", fp);
  else
    fputs("chHV in (kV),\n", fp);
  fputs("chCurrentLimit in (mA)\n", fp);
  fputs("</comment>\n", fp);
  // write device name
  fputs("<hvDeviceName>\n", fp);
  str = device+"\n";
  fputs((char *)str.ascii(), fp);
  fputs("</hvDeviceName>\n", fp);
  // write HV data -----------------------------------------
  // get device list offset
  offset = 0;
  for (i=0; i<f_nChannels; i++) {
    str = fName+i*f_nNameLength;
    if (str.find(device, 0, FALSE) != -1) { // found
      break;
    }
    offset++;
  }
  // write data
  for (i=0; i<f_nChannels; i++) {
    str = fName+i*f_nNameLength; // get name, e.g. FUG%Moderator
    if (str.find(device, 0, FALSE) != -1) { // correct device
      sub = strstr(fName+i*f_nNameLength,"%"); // goto '%'
      sub++; // move to the begining of the channnel name
      str = "<hvCh name=\""+QString(sub)+"\">\n";
      fputs((char *)str.ascii(), fp);
      // channel no
      str = QString("  <chNo>%1</chNo>\n").arg(i-offset+1);
      fputs((char *)str.ascii(), fp);
      // name
      str = "  <name>"+QString(sub)+"</name>\n";
      fputs((char *)str.ascii(), fp);
      // hv demand
      str = QString("  <hvDemand>%1</hvDemand>\n").arg(fDemand[i]);
      fputs((char *)str.ascii(), fp);
      // hv measured
      str = QString("  <hvMeasured>%1</hvMeasured>\n").arg(fMeasured[i]);
      fputs((char *)str.ascii(), fp);
      // current limit
      str = QString("  <currentLimit>%1</currentLimit>\n").arg(fCurrentLimit[i]);
      fputs((char *)str.ascii(), fp);
      fputs("</hvCh>\n", fp);
    }
  }
  // write end tag
  fputs("</HV>\n", fp);

  // close file
  fclose(fp);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of FilePrint
//*******************************************************************************************************
/*!
 * <p>Prints the current HV table including the current limit settings.
 */
void PHvEdit::FilePrint()
{
  int     i, offset;
  QString device, str;
  char    *name, line[128];
  QDate   date = QDate::currentDate();
  QTime   time = QTime::currentTime();

  QPrinter *printer = new QPrinter;
  const int kMargin = 45;

  // get current device group
  device = fDevice_listBox->currentText();

  // get device list offset
  offset = 0;
  for (i=0; i<f_nChannels; i++) {
    str = fName+i*f_nNameLength;
    if (str.find(device, 0, FALSE) != -1) { // found
      break;
    }
    offset++;
  }

  if (printer->setup(this)) { // printer dialog
     statusBar()->message( "Printing..." );

     QPainter p;
     if(!p.begin(printer))                  // paint on printer
       return;

     QFont f("Courier");
     p.setFont(f);

     int yPos        = 0;                    // y-position for each line
     QFontMetrics fm = p.fontMetrics();
     QPaintDeviceMetrics metrics( printer ); // need width/height

     // write header
     p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                ExpandTabs | DontClip, "-------------------------------------------------------------------------");
     yPos += fm.lineSpacing();
     p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                ExpandTabs | DontClip, ">> "+time.toString()+", "+date.toString("dddd, MMMM d, yyyy"));
     yPos += fm.lineSpacing();
     p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                ExpandTabs | DontClip, "-------------------------------------------------------------------------");
     yPos += fm.lineSpacing();
     p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                ExpandTabs | DontClip, "Device Name = "+device);
     yPos += fm.lineSpacing();
     p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                ExpandTabs | DontClip, "-------------------------------------------------------------------------");
     yPos += fm.lineSpacing();
     p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                ExpandTabs | DontClip, "Ch No,  Demand,  Measured,  Current,  CurrentLimit,  Name");
     yPos += fm.lineSpacing();
     if (fDemandInV)
       p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                  ExpandTabs | DontClip, "         (V)       (V)        (mA)        (mA)");
     else
       p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                  ExpandTabs | DontClip, "         (kV)      (kV)       (mA)        (mA)");
     yPos += fm.lineSpacing();

     // write data
     for (i=0; i<f_nChannels; i++) {
       if ( kMargin + yPos > metrics.height() - kMargin ) { // new page needed?
         printer->newPage();             // no more room on this page
         yPos = 0;                       // back to top of page
       }
       str = fName+i*f_nNameLength;
       if (str.find(device, 0, FALSE) != -1) { // found
         name = strstr(fName+i*f_nNameLength, "%");
         name++;
         if (fDemandInV)
           sprintf(line, "%02d,    %+0.2f,    %+0.2f,     %+0.3f,    %+0.3f,    %s", i-offset+1,
                   fDemand[i], fMeasured[i], fCurrent[i], fCurrentLimit[i], name);
         else
           sprintf(line, "%02d,    %+0.1f,    %+0.1f,     %+0.3f,    %+0.3f,    %s", i-offset+1,
                   fDemand[i], fMeasured[i], fCurrent[i], fCurrentLimit[i], name);
         str = line;
         p.drawText(kMargin, kMargin+yPos, metrics.width(), fm.lineSpacing(),
                    ExpandTabs | DontClip, str);
         yPos += fm.lineSpacing();
       }
     }

     p.end();                                // send job to printer
     statusBar()->message( "Printing completed", 2000 );
  } else {
    statusBar()->message( "Printing aborted", 2000 );
  }

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of FileExit
//*******************************************************************************************************
/*!
 * <p>Will pop-up a termination dialog and, if wished, disconnect from the experiment and terminate
 * the program.
 */
void PHvEdit::FileExit()
{
  int ans;

  if (fExp->IsConnected()) {
    ans = QMessageBox::warning(0, "WARNING", "You are about to exit hvEdit.\nDo you really want to do that?",
                               QMessageBox::Yes, QMessageBox::No);
    if (ans == QMessageBox::No) {
      return ;
    }
    DisconnectFromExp();
  }
  exit(CM_SUCCESS);
}

//*******************************************************************************************************
//  implementation of HelpIndex
//*******************************************************************************************************
/*!
 * <p>The help index is not enabled yet.
 */
void PHvEdit::HelpIndex()
{
  QMessageBox::information(0, "INFO", "help index not implemented yet.", QMessageBox::Ok);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of helpContents
//*******************************************************************************************************
/*!
 * <p>Starts the hvEdit help browser.
 */
void PHvEdit::HelpContents()
{

  QString home = *fHvA->GetHvDefaultDoc()+"/hvEdit_Help.html";

  PHelpWindow *help = new PHelpWindow(home, *fHvA->GetHvDefaultDoc(), 0, "help viewer");
  help->setCaption("hvEdit - Helpviewer");
  help->show();

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of HelpAbout
//*******************************************************************************************************
/*!
 * <p>Starts the about popup window.
 */
void PHvEdit::HelpAbout()
{
  PHvEditAbout *hvEditAbout = new PHvEditAbout();
  hvEditAbout->show();

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of HelpAboutQt
//*******************************************************************************************************
/*!
 * <p>Starts the about-Qt popup window.
 */
void PHvEdit::HelpAboutQt()
{
  QMessageBox::aboutQt(this, "a Qt application.");

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of ConnectToExp
//*******************************************************************************************************
/*!
 * <p>Connects to the experiment and opens all the necessary keys. Also populates the HV/CurrentLimit
 * table.
 */
int PHvEdit::ConnectToExp()
{
  // try to connect to the experiment
  if (!fExp->Connect(fExp->GetHostName(), fExp->GetExperimentName(), fExp->GetClientName()))
    return 0;

  // set names Host and Experiment in hvEdit GUI
  SetHostName(fExp->GetHostName());
  SetExpName(fExp->GetExperimentName());

  // open keys
  if (!OpenKeys()) {
    QMessageBox::warning(0, "hvEdit ERROR", "Cannot get requiered Keys!",
                         QMessageBox::Ok, QMessageBox::NoButton);
    // disconnect from experiment
    DisconnectFromExp();
    // clear host and experiment name
    fExp->SetHostName("");
    fExp->SetExperimentName("");
    SetHostName(fExp->GetHostName());
    SetExpName(fExp->GetExperimentName());
    return 0;
  }

  if (!UpdateChannelDefinitions()) return 0;

  UpdateTable(-1, true);
  EnableConnected();

  return 1;
}

//*******************************************************************************************************
//  implementation of DisconnectToExp
//*******************************************************************************************************
/*!
 * <p>Disconnects from the experiment and cleans up the HV/CurrentLimit tables.
 */
void PHvEdit::DisconnectFromExp()
{
  CloseKeys();
  // reset group list of the GUI
  fDevice_listBox->clear();
  fExp->Disconnect();
  // remove names Host and Experiment from hvEdit GUI
  SetHostName("");
  SetExpName("");
  ClearTable();
  // remove any selection from table (GUI)
  fHv_table->clearSelection();
}

//*******************************************************************************************************
//  implementation of ChangedDeviceSelection (SLOT)
//*******************************************************************************************************
/*!
 * <p>Called when the device selection changes. It then updates the HV/CurrentLimit tables so that
 * they will display the new device.
 */
void PHvEdit::ChangedDeviceSelection()
{
  f_iGroup = fDevice_listBox->currentItem();
  if (fDemand) { // i.e. hotlink already established
    UpdateTable(-1, true);
  }

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnSelectAll (SLOT)
//*******************************************************************************************************
/*!
 * <p>Will select all channels of the current device
 */
void PHvEdit::OnSelectAll()
{
  //hve_HV_table->selectColumn(HV_DEMAND); // Qt3.1

  // Qt3.0.3 equivalent to the above Qt3.1 command
  QTableSelection s;
  s.init(0,HV_DEMAND); // anchor selection
  s.expandTo(fHv_table->numRows(),HV_DEMAND); // expand to last element in the HV_DEMAND column
  fHv_table->addSelection(s);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnSwitchAllChannelOff (SLOT)
//*******************************************************************************************************
/*!
 * <p>Called when the 'Switch Off All Channels' button is pushed. It will first popup a dialog
 * to make sure that the user really wants to shut down all HV's of <b>all</b> devices. If yes,
 * all HV channels of all devices will set to zero.
 */
void PHvEdit::OnSwitchAllChannelsOff()
{
  int result, i;

  result = QMessageBox::warning(0, "WARNING", "Are you SURE to set all channels in all groups to zero?",
                                QMessageBox::Yes, QMessageBox::No);

  if (result == QMessageBox::Yes) {
    for (i=0 ; i<f_nChannels ; i++) {
      if (fDemand[i] > 0)
        fRestore[i] = fDemand[i];
      fDemand[i] = 0.f;
    }

    UpdateOdbDemand(-1);
    UpdateTable(-1, false);
  }

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnZero (SLOT)
//*******************************************************************************************************
/*!
 * <p>Will set the selected channels to zero.
 */
void PHvEdit::OnZero()
{
  int i, j;
  int device;

  // get device selection
  device = fDevice_listBox->currentItem();
  j = 0; // device channel counter
  for (i=0; i<f_nChannels; i++) { // i = all over channel counter
    if (fGroup[i] == device) { // check if correct device
      if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
        if (fHv_table->isSelected(j,HV_DEMAND)) { // get current selection
          // save voltages in restore buffer
          fRestore[i] = fDemand[i];
          // set voltage to zero
          fDemand[i] = 0.0f;
        }
      } else { // current limit tab
        QMessageBox::warning(0, "WARNING", "This is NOT allowed for current limit",
                             QMessageBox::Ok, QMessageBox::NoButton);
        return ;
      }
      j++;
    }
  }
  UpdateOdbDemand(-1);
  UpdateTable(-1, false);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnRestore (SLOT)
//*******************************************************************************************************
/*!
 * <p>Will restore the HV's of the current selection, i.e. set them to their previous value.
 */
void PHvEdit::OnRestore()
{
  int   i, j;
  int   device;
  float value;

  // get device selection
  device = fDevice_listBox->currentItem();
  j = 0; // device channel counter
  for (i=0; i<f_nChannels; i++) { // i = all over channel counter
    if (fGroup[i] == device) { // check if correct device
      if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
        if (fHv_table->isSelected(j,HV_DEMAND)) { // get current selection
          value = fDemand[i];
          // retrieve voltages from restore buffer
          fDemand[i] = fRestore[i];
          // writes the previous demand value into the restore buffer
          fRestore[i] = value;
        }
      } else { // current limit tab
        return ;
      }
      j++;
    }
  }
  UpdateOdbDemand(-1);
  UpdateTable(-1, false);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnM100 (SLOT)
//*******************************************************************************************************
/*!
 * <p>Decrements the selected channels by 100V (fDemandInV=true) and 1.0 kV (fDemandInV=false), repectivly
 */
void PHvEdit::OnM100()
{
  if (fDemandInV)
    Increment(-100.0f);
  else
    Increment(-1.0f);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnM010 (SLOT)
//*******************************************************************************************************
/*!
 * <p>Decrements the selected channels by 10V (fDemandInV=true) and 0.1 kV (fDemandInV=false), repectivly
 */
void PHvEdit::OnM010()
{
  if (fDemandInV)
    Increment(-10.0f);
  else
    Increment(-0.1f);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnM001 (SLOT)
//*******************************************************************************************************
/*!
 * <p>Decrements the selected channels by 1V (fDemandInV=true) and 0.01 kV (fDemandInV=false), repectivly
 */
void PHvEdit::OnM001()
{
  if (fDemandInV)
    Increment(-1.0f);
  else
    Increment(-0.01f);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnP100 (SLOT)
//*******************************************************************************************************
/*!
 * <p>Increments the selected channels by 100V (fDemandInV=true) and 1.0 kV (fDemandInV=false), repectivly
 */
void PHvEdit::OnP100()
{
  if (fDemandInV)
    Increment(100.0f);
  else
    Increment(1.0f);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnP010 (SLOT)
//*******************************************************************************************************
/*!
 * <p>Increments the selected channels by 10V (fDemandInV=true) and 0.1 kV (fDemandInV=false), repectivly
 */
void PHvEdit::OnP010()
{
  if (fDemandInV)
    Increment(10.0f);
  else
    Increment(0.1f);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnP001 (SLOT)
//*******************************************************************************************************
/*!
 * <p>Increments the selected channels by 1V (fDemandInV=true) and 0.01 kV (fDemandInV=false), repectivly
 */
void PHvEdit::OnP001()
{
  if (fDemandInV)
    Increment(1.0f);
  else
    Increment(0.01f);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnSet (SLOT)
//*******************************************************************************************************
/*!
 * <p>Extracts the value from the input text field and sets the selected demand/current limit values.
 */
void PHvEdit::OnSet()
{
  int i, j;
  int device;
  QString str;

  str = fInput_comboBox->currentText();
  // get device selection
  device = fDevice_listBox->currentItem();
  j = 0; // device channel counter
  for (i=0; i<f_nChannels; i++) { // i = all over channel counter
    if (fGroup[i] == device) { // check if correct device
      if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
        if (fHv_table->isSelected(j,HV_DEMAND)) { // get current selection
          // keep restore value
          fRestore[i] = fDemand[i];
          // set voltage
          fDemand[i] = str.toFloat();
        }
      } else { // current limit tab
        if (fCurrentLimit_table->isSelected(j,CL_CURRENT_LIMIT)) { // get current selection
          // set current limit
          fCurrentLimit[i] = str.toFloat();
        }
      }
      j++;
    }
  }
  if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
    UpdateOdbDemand(-1);
  } else { // current limit tab
    UpdateOdbCurrentLimit(-1);
  }

  UpdateTable(-1, false);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of OnScale (SLOT)
//*******************************************************************************************************
/*!
 * <p>Extracts the value from the 'scale by' combo box and sets the selected demand/current limit values.
 */
void PHvEdit::OnScale()
{
  float scale = fScale_comboBox->currentText().toFloat(); // read comboBox value
  int   device, i, j;


  // check if selection is HV and not 'current limit'
  if (fHv_tabWidget->currentPageIndex() != 0) { // i.e. not HV demand tab
    QMessageBox::critical(0, "ERROR", "You cannot only scale 'demand HV'.", QMessageBox::Ok, QMessageBox::NoButton);
    return;
  }

  // check if scale is in reasonable bounds
  if (scale < 0.f) {
    QMessageBox::critical(0, "ERROR", "You cannot scale by negative values!", QMessageBox::Ok, QMessageBox::NoButton);
    return;
  }
  if (scale > 2.f) {
    if ( QMessageBox::warning(0, "WARNING", "<b><p>You are going to scale by "
            +fScale_comboBox->currentText()+".<p>Are you serious about it?<\b>",
           QMessageBox::Yes, QMessageBox::No) == QMessageBox::No )
      return;
  }

  // get device selection
  device = fDevice_listBox->currentItem();
  j = 0; // device channel counter
  for (i=0; i<f_nChannels; i++) { // i = all over channel counter
    if (fGroup[i] == device) { // check if correct device
      if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
        if (fHv_table->isSelected(j,HV_DEMAND)) { // get HV selection
          // save current value into restore buffer
          fRestore[i] = fDemand[i];
          // set voltage
          fDemand[i] *= scale;
        }
      }
      j++;
    }
  }
  if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
    UpdateOdbDemand(-1);
    UpdateTable(-1, false);
  }

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of RestartTimeoutTimer 
//*******************************************************************************************************
/*!
 * <p>restarts the timeout timer if the given timeout is > 0.
 */
void PHvEdit::RestartTimeoutTimer()
{
  if (fHvA->GetTerminationTimeout() > 0) { // only start termination timeout handler if timeout > 0
    // restart timeout timer
    fTerminationTimer->start(1000*60*fHvA->GetTerminationTimeout());
  }
}

//*******************************************************************************************************
//  implementation of HvValueChanged (SLOT)
//*******************************************************************************************************
/*!
 * <p>This function is called when the user has changed a value in the HV demand settings.
 *
 * \param row of the changed HV value
 * \param col HV column, which should be only the demand value one
 */
void PHvEdit::HvValueChanged(int row, int col)
{
  int i, j;
  int device;
  QString str;

  if (col != HV_DEMAND) {
    QMessageBox::warning(0, "WARNING", "Wrong column, somthing is VERY fishy",
                         QMessageBox::Ok, QMessageBox::NoButton);
    return ;
  }
  // get device selection
  device = fDevice_listBox->currentItem();
  j = 0; // device channel counter
  for (i=0; i<f_nChannels; i++) { // i = all over channel counter
    if (fGroup[i] == device) { // check if correct device
      if (j == row) { // got changed item
        // get value from table
        str = fHv_table->text(row, col);
        fDemand[i] = str.toFloat();
      }
      j++;
    }
  }
  UpdateOdbDemand(-1);
  UpdateTable(-1, false);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of CurrentLimitValueChanged (SLOT)
//*******************************************************************************************************
/*!
 * <p>This function is called when the user has changed a value in the current limit settings.
 *
 * \param row row of the current limit, which has changed
 * \param col current limit column
 */
void PHvEdit::CurrentLimitValueChanged(int row, int col)
{
  int i, j;
  int device;
  QString str;

  if (col != CL_CURRENT_LIMIT) {
    QMessageBox::warning(0, "WARNING", "Wrong column, somthing is VERY fishy",
                         QMessageBox::Ok, QMessageBox::NoButton);
    return ;
  }
  // get device selection
  device = fDevice_listBox->currentItem();
  j = 0; // device channel counter
  for (i=0; i<f_nChannels; i++) { // i = all over channel counter
    if (fGroup[i] == device) { // check if correct device
      if (j == row) { // got changed item
        // get value from table
        str = fCurrentLimit_table->text(row, col);
        fCurrentLimit[i] = str.toFloat();
      }
      j++;
    }
  }
  UpdateOdbCurrentLimit(-1);
  UpdateTable(-1, false);

  RestartTimeoutTimer();
}

//*******************************************************************************************************
//  implementation of EnableConnected
//*******************************************************************************************************
/*!
 * <p>Enables all the necessary buttons and actions.
 */
void PHvEdit::EnableConnected()
{
  fileOpenAction->setEnabled(TRUE);
  fileSaveAction->setEnabled(TRUE);
  filePrintAction->setEnabled(TRUE);

  fSwitchAllOff_pushButton->setEnabled(TRUE);
  fSet_pushButton->setEnabled(TRUE);
  fM100_pushButton->setEnabled(TRUE);
  fM010_pushButton->setEnabled(TRUE);
  fM001_pushButton->setEnabled(TRUE);
  fP100_pushButton->setEnabled(TRUE);
  fP010_pushButton->setEnabled(TRUE);
  fP001_pushButton->setEnabled(TRUE);
  fZero_pushButton->setEnabled(TRUE);
  fRestore_pushButton->setEnabled(TRUE);
  fSelectAll_pushButton->setEnabled(TRUE);
  fScale_pushButton->setEnabled(TRUE);
  fScale_comboBox->setEnabled(TRUE);
}

//*******************************************************************************************************
//  implementation of OpenKeys
//*******************************************************************************************************
/*!
 * <p>Tries to open all the necessary ODB keys
 */
BOOL PHvEdit::OpenKeys()
{
  int i;

  // Root key
  for (i=0; i<fHvA->fNoOdbRoots; i++) {
    fRootKey->append(new PKey(fExp, fHvA->GetHvOdbRoot(i)));
    if (!fRootKey->at(i)->IsValid())
      return FALSE;
  }

  // Names key
  for (i=0; i<fHvA->fNoOdbRoots; i++) {
    fNamesKey->append(new PKey(fRootKey->at(i), fHvA->GetHvOdbNames()));
    if (!fNamesKey->at(i)->IsValid())
      return FALSE;
  }

  // Demand key
  for (i=0; i<fHvA->fNoOdbRoots; i++) {
    fDemandKey->append(new PKey(fRootKey->at(i), fHvA->GetHvOdbDemand()));
    if (!fDemandKey->at(i)->IsValid())
      return FALSE;
  }

  // Measured key
  for (i=0; i<fHvA->fNoOdbRoots; i++) {
    fMeasuredKey->append(new PKey(fRootKey->at(i), fHvA->GetHvOdbMeasured()));
    if (!fMeasuredKey->at(i)->IsValid())
      return FALSE;
  }

  // Current key
  for (i=0; i<fHvA->fNoOdbRoots; i++) {
    fCurrentKey->append(new PKey(fRootKey->at(i), fHvA->GetHvOdbCurrent()));
    if (!fCurrentKey->at(i)->IsValid())
      return FALSE;
  }

  // Current Limit key
  for (i=0; i<fHvA->fNoOdbRoots; i++) {
    fCurrentLimitKey->append(new PKey(fRootKey->at(i), fHvA->GetHvOdbCurrentLimit()));
    if (!fCurrentLimitKey->at(i)->IsValid())
      return FALSE;
  }

  return TRUE;
}

//*******************************************************************************************************
//  implementation of CloseKeys
//*******************************************************************************************************
/*!
 * <p>Closes all open ODB keys.
 */
void PHvEdit::CloseKeys()
{
  if (fRootKey) {
    delete fRootKey;
    fRootKey = new QPtrList<PKey>(); // reinitialize
  }
  if (fNamesKey) {
    delete fNamesKey;
    fNamesKey = new QPtrList<PKey>(); // reinitialize
  }
  if (fDemandKey) {
    delete fDemandKey;
    fDemandKey = new QPtrList<PKey>(); // reinitialize
  }
  if (fMeasuredKey) {
    delete fMeasuredKey;
    fMeasuredKey = new QPtrList<PKey>(); // reinitialize
  }
  if (fCurrentKey) {
    delete fCurrentKey;
    fCurrentKey = new QPtrList<PKey>(); // reinitialize
  }
  if (fCurrentLimitKey) {
    delete fCurrentLimitKey;
    fCurrentLimitKey = new QPtrList<PKey>(); // reinitialize
  }
}

//*******************************************************************************************************
//  implementation of FreeMem
//*******************************************************************************************************
/*!
 * <p>free's all the allocated memory
 */
void PHvEdit::FreeMem()
{
  // free local memory
  if (fSelection) {
    free(fSelection);
    free(fGroup);
    free(fOdbRootMap);
    free(fRestore);
    free(fDemand);
    free(fDemandCache);
    free(fMeasured);
    free(fMeasuredCache);
    free(fCurrent);
    free(fCurrentCache);
    free(fCurrentLimit);
    free(fCurrentLimitCache);
    fSelection = NULL;
  }
}

//*******************************************************************************************************
//  implementation of ChannelIndex
//*******************************************************************************************************
/*!
 * <p>Find index to fDemand etc. according to selected channel 'ch' and group 'f_iGroup'.
 *
 * \param ch channel of the current device
 */
int PHvEdit::ChannelIndex(int ch)
{
  int i,j;

  // Find index to fDemand etc. according to selected
  // channel 'ch' and group 'f_iGroup'

  for (i=0,j=0 ; i<f_nChannels ; i++) {
    if (fGroup[i] == f_iGroup)
      j++;
    if (j == ch+1)
      return i;
  }

  return 0;
}

//*******************************************************************************************************
//  implementation of ClearTable
//*******************************************************************************************************
/*!
 * <p>Clears the table, i.e. removes all items
 */
void PHvEdit::ClearTable()
{
  int i, j;
  for (i=0; i<fHv_table->numCols(); i++) {
    for (j=0; j<fHv_table->numRows(); j++) {
      fHv_table->clearCell(j,i);
    }
  }
  for (i=0; i<fCurrentLimit_table->numCols(); i++) {
    for (j=0; j<fCurrentLimit_table->numRows(); j++) {
      fCurrentLimit_table->clearCell(j,i);
    }
  }
}

//*******************************************************************************************************
//  implementation of UpdateOdbDemand
//*******************************************************************************************************
/*!
 * <p>Sets HV demand values in the ODB
 *
 * \param ch if ch > 0 it updates only this value, otherwise it is updating all the HV demand values.
 */
void PHvEdit::UpdateOdbDemand(int ch)
{
  int status;
  int size;
  int rootNo;
  int offset;

  // Update demand values in database
  if (ch >= 0) {
    rootNo = fOdbRootMap[ch];
    // calculate correct demand value address
    offset = 0;
    for (int i=0; i<rootNo; i++)
      offset += fDemandKey->at(i)->GetNumValues();
    status = fDemandKey->at(rootNo)->SetDataIndex(fDemand+ch, ch-offset, TID_FLOAT);
  } else {
    offset = 0;
    for (int i=0; i<fHvA->fNoOdbRoots; i++) {
      status = fDemandKey->at(i)->SetData(&fDemand[offset], fDemandKey->at(i)->GetNumValues(), TID_FLOAT);
      offset += fDemandKey->at(i)->GetNumValues();
    }
  }

  if (status == DB_NO_ACCESS) {
    offset = 0;
    for (int i=0; i<fHvA->fNoOdbRoots; i++) {
      size = fDemandKey->at(i)->GetNumValues() * sizeof(float);
      fDemandKey->at(i)->GetData(&fDemand[offset], &size, TID_FLOAT);
      offset += fDemandKey->at(i)->GetNumValues();
    }
    MidasMessage("Change of demand values currently not allowed");
  }
}

//*******************************************************************************************************
//  implementation of UpdateOdbCurrentLimit
//*******************************************************************************************************
/*!
 * <p>Sets current limit values in the ODB
 *
 * \param ch if ch > 0 it updates only this value, otherwise it is updating all the current limit values.
 */
void PHvEdit::UpdateOdbCurrentLimit(int ch)
{
  int status, size;
  int rootNo;
  int offset;

  // Update current limit values in database
  if (ch >= 0) {
    rootNo = fOdbRootMap[ch];
    // calculate correct demand value address
    offset = 0;
    for (int i=0; i<rootNo; i++)
      offset += fDemandKey->at(i)->GetNumValues();
    status = fCurrentLimitKey->at(rootNo)->SetDataIndex(fCurrentLimit+ch, ch-offset, TID_FLOAT);
  } else {
    offset = 0;
    for (int i=0; i<fHvA->fNoOdbRoots; i++) {
      status = fCurrentLimitKey->at(i)->SetData(&fCurrentLimit[offset],
                                                fCurrentLimitKey->at(i)->GetNumValues(), TID_FLOAT);
      offset += fDemandKey->at(i)->GetNumValues();
    }
  }

  if (status == DB_NO_ACCESS) {
    offset = 0;
    for (int i=0; i<fHvA->fNoOdbRoots; i++) {
      size = fDemandKey->at(i)->GetNumValues() * sizeof(float);
      fCurrentLimitKey->at(i)->GetData(&fCurrentLimit[offset], &size, TID_FLOAT);
      offset += fDemandKey->at(i)->GetNumValues();
    }
    MidasMessage("Change of current limit values currently not allowed");
  }
}

//*******************************************************************************************************
//  implementation of Increment
//*******************************************************************************************************
/*!
 * <p>Increment the selected cells by the amount given by incr
 *
 * \param incr value to be incremented
 */
void PHvEdit::Increment(const float incr)
{
  int i, j;
  int device;

  // get device selection
  device = fDevice_listBox->currentItem();
  j = 0; // device channel counter
  for (i=0; i<f_nChannels; i++) { // i = all over channel counter
    if (fGroup[i] == device) { // check if correct device
      if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
        if (fHv_table->isSelected(j,HV_DEMAND)) { // get HV selection
          // save current value into restore buffer
          fRestore[i] = fDemand[i];
          // set voltage
          fDemand[i] += incr;
        }
      } else { // current limit tab
        if (fCurrentLimit_table->isSelected(j,CL_CURRENT_LIMIT)) { // get current selection
          // set current limit
          fCurrentLimit[i] += incr;
        }
      }
      j++;
    }
  }
  if (fHv_tabWidget->currentPageIndex() == 0) { // i.e. HV demand tab
    UpdateOdbDemand(-1);
  } else { // current limit tab
    UpdateOdbCurrentLimit(-1);
  }

  UpdateTable(-1, false);
}

//*******************************************************************************************************
// DisconnectAndQuit (SLOT)
//*******************************************************************************************************
/*!
 * <p>Called when disconnect from the experiment.
 */
void PHvEdit::DisconnectAndQuit()
{
  DisconnectFromExp();
  QApplication::exit(0);
}

//*******************************************************************************************************
// NetworkProblems (SLOT)
//*******************************************************************************************************
/*!
 * <p>Called when network problems occure.
 */
void PHvEdit::NetworkProblems()
{
  statusBar()->message(QString("Network Problems ..."), 2000);
}

//*******************************************************************************************************
//  implementation of NamesChanged
//    A member function cannot be passed to db_open_record. Therefore, NamesChanged
//    works as a stub to call updateChannelDefinition
//*******************************************************************************************************
/*!
 * <p>Hotlink routine called when the Names are changed.
 */
void NamesChanged(int, int, void *info)
{
  PHvEdit *dlg;

  dlg = (PHvEdit *) info;
  dlg->UpdateChannelDefinitions();
  dlg->UpdateTable(-1, true);
}

//*******************************************************************************************************
//  implementation of HVChanged
//*******************************************************************************************************
/*!
 * <p>This function gets linked to the open records (demand and measured)
 *    by the db_open_record calls. Whenever a record changes, cm_yield
 *    first updates the local copy of the record and then calls odb_update
 *
 * \param info pointer to the PHvEdit object.
 */
void HVChanged(HNDLE, HNDLE, void *info)
{
  PHvEdit  *dlg;

  dlg = (PHvEdit *) info;
  if (dlg == NULL) {
    MidasMessage(">> HVChanges: Couldn't get Main Window!!");
  } else {
    dlg->UpdateTable(-1, false);
  }
}
