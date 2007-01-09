/*******************************************************************************************

  PHvAdmin.cpp

  Administration class to encapsulate midas online data base (ODB) settings.

  Sample hvEdit.xml file:

   <?xml version="1.0" encoding="UTF-8"?>
   <hvEdit xmlns="http://midas.web.psi.ch/hvEdit">
     <comment>
       hvEdit is looking for this file under $MIDASSYS/gui/hvedit/qt-3.3/hvEdit/bin/xmls
       where $MIDASSYS is the source directory of midas

      general:                general settings for hvEdit
        default_settings_dir: the path to the hv settings files
        default_doc_dir:      the path to the hvEdit help files
        termination_timeout:  timeout in (min). If within this timeout there is no user action,
                              hvEdit will terminate. Setting termination_timeout to a negative
                              number means that this feature will be disabled.
        demand_in_V:          flag showing if the demand HV's are in V (true) or kV (false)
      odb_key:                MIDAS online data base (ODB) pathes or realtive pathes
        hv_root:              ODB path to the HV root. Various HV roots are allowed, though the
                              number must be entered in the general section with the label
                              no_of_hv_odb_roots.
        hv_names:             relative ODB path where to find the channel names
        hv_demand:            relative ODB path where to find the hv demand values
        hv_measured:          relative ODB path where to find the hv measured values
        hv_current:           relative ODB path where to find the measured currents
        hv_current_limit:     relative ODB path where to find the current limits
     </comment>
     <general>
       <default_settings_dir>/home/nemu/midas/experiment/nemu/hv_settings</default_settings_dir>
       <default_doc_dir>/home/nemu/midas/lemQt/hvEdit/doc</default_doc_dir>
       <termination_timeout>15</termination_timeout>
       <demand_in_V>true</demand_in_V>
     </general>
     <odb_keys>
       <hv_root>/Equipment/HV</hv_root>
       <hv_root>/Equipment/HV Detectors</hv_root>
       <hv_names>Settings/Names</hv_names>
       <hv_demand>Variables/Demand</hv_demand>
       <hv_measured>Variables/Measured</hv_measured>
       <hv_current>Variables/Current</hv_current>
       <hv_current_limit>Settings/Current Limit</hv_current_limit>
     </odb_keys>
   </hvEdit>

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

#include <qstring.h>
#include <qfile.h>

#include "midas.h"

#include "PHvAdmin.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// implementation of hvAdminXMLParser
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//**********************************************************************
// constructor
//**********************************************************************
/*!
 * <p>XML Parser class for the hvEdit administration file.
 *
 * \param hva pointer to the main class.
 */
PHvAdminXMLParser::PHvAdminXMLParser(PHvAdmin *hva)
{
  fHva = hva;
  fKey = empty;
  fOdbRootCounter = 0;
}

//**********************************************************************
// startDocument
//**********************************************************************
/*!
 * <p>Routine called at the beginning of the XML parsing process.
 */
bool PHvAdminXMLParser::startDocument()
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
bool PHvAdminXMLParser::startElement( const QString&, const QString&,
                                       const QString& qName,
                                       const QXmlAttributes& )
{
  if (qName == "default_settings_dir") {
    fKey = defaultSettingDir;
  } else if (qName == "default_doc_dir") {
    fKey = defaultDocDir;
  } else if (qName == "termination_timeout") {
    fKey = terminationTimeout;
  } else if (qName == "demand_in_V") {
    fKey = demandInV;
  } else if (qName == "hv_root") {
    if (fOdbRootCounter == 0)
      fHva->fMidasOdbHvRoot->remove();
    fOdbRootCounter++;
    fKey = hvRoot;
  } else if (qName == "hv_names") {
    fKey = hvNames;
  } else if (qName == "hv_demand") {
    fKey = hvDemand;
  } else if (qName == "hv_measured") {
    fKey = hvMeasured;
  } else if (qName == "hv_current") {
    fKey = hvCurrent;
  } else if (qName == "hv_current_limit") {
    fKey = hvCurrentLimit;
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
bool PHvAdminXMLParser::endElement( const QString&, const QString&, const QString& )
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
bool PHvAdminXMLParser::characters(const QString& str)
{
  QString root;

  switch (fKey) {
    case defaultSettingDir:
      fHva->fDefaultDirHvSettings = QString(str.ascii()).stripWhiteSpace();
      fHva->fb_DefaultDirHvSettings = true;
      break;
    case defaultDocDir:
      fHva->fDefaultDirDocu = QString(str.ascii()).stripWhiteSpace();
      fHva->fb_DefaultDirDocu = true;
      break;
    case terminationTimeout:
      fHva->fTerminationTimeout = str.toInt();
      fHva->fb_TerminationTimeout = true;
      break;
    case demandInV:
      if (str == "true")
        fHva->fDemandInV = true;
      else
        fHva->fDemandInV = false;
      fHva->fb_DemandInVPresent = true;
      break;
    case hvRoot:
      root = QString(str.ascii()).stripWhiteSpace();
      fHva->fMidasOdbHvRoot->append(new QString(root));
      fHva->fb_MidasOdbHvRoot = true;
      break;
    case hvNames:
      fHva->fMidasOdbHvNames = QString(str.ascii()).stripWhiteSpace();
      fHva->fb_MidasOdbHvNames = true;
      break;
    case hvDemand:
      fHva->fMidasOdbHvDemand = QString(str.ascii()).stripWhiteSpace();
      fHva->fb_MidasOdbHvDemand = true;
      break;
    case hvMeasured:
      fHva->fMidasOdbHvMeasured = QString(str.ascii()).stripWhiteSpace();
      fHva->fb_MidasOdbHvMeasured = true;
      break;
    case hvCurrent:
      fHva->fMidasOdbHvCurrent = QString(str.ascii()).stripWhiteSpace();
      fHva->fb_MidasOdbHvCurrent = true;
      break;
    case hvCurrentLimit:
      fHva->fMidasOdbHvCurrentLimit = QString(str.ascii()).stripWhiteSpace();
      fHva->fb_MidasOdbHvCurrentLimit = true;
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
bool PHvAdminXMLParser::endDocument()
{
  fHva->fNoOdbRoots = fOdbRootCounter;

  return TRUE;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// implementation of PHvAdmin
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//**********************************************************************
// PHvAdmin constructor
//**********************************************************************
/*!
 * <p>Constructs the administration class object. This class holds all ODB and
 * path related variables.
 *
 * \param host name of the host
 * \param exp  name of the experiment
 */
PHvAdmin::PHvAdmin(char *host, char *exp)
{
  fHost = QString(host).stripWhiteSpace();
  fExp  = QString(exp).stripWhiteSpace();

  // initialize flags for ODB pathes
  fb_DefaultDirHvSettings    = false;
  fb_DefaultDirDocu          = false;
  fb_TerminationTimeout      = false;
  fb_DemandInVPresent        = false;
  fb_MidasOdbHvRoot          = false;
  fb_MidasOdbHvNames         = false;
  fb_MidasOdbHvDemand        = false;
  fb_MidasOdbHvMeasured      = false;
  fb_MidasOdbHvCurrent       = false;
  fb_MidasOdbHvCurrentLimit  = false;

  // init counters
  fNoOdbRoots = 0;

  // init termination timeout to a 15 (min) default
  fTerminationTimeout = 15;
  
  // init demand in V default
  fDemandInV = true;

  // init ODB path strings to some sensible defaults
  QString str("/Equipment/HV");
  fMidasOdbHvRoot = new QPtrList<QString>();
  fMidasOdbHvRoot->setAutoDelete(true);
  fMidasOdbHvRoot->append(new QString(str));

  QString midasRoot;
  if (getenv("MIDASSYS"))
    midasRoot = QString(getenv("MIDASSYS"));
  else
    midasRoot = QString(".");
  
  fDefaultDirHvSettings   = midasRoot + QString("/gui/hvedit/qt-3.3/hvEdit/hv_settings");
  fDefaultDirDocu         = midasRoot + QString("/gui/hvedit/qt-3.3/hvEdit/doc");
  fMidasOdbHvNames        = QString("Settings/Names");
  fMidasOdbHvDemand       = QString("Variables/Demand");
  fMidasOdbHvMeasured     = QString("Variables/Measured");
  fMidasOdbHvCurrent      = QString("Variables/Current");
  fMidasOdbHvCurrentLimit = QString("Settings/Current Limit");

  // XML Parser part
  QString fln = midasRoot+"/gui/hvedit/qt-3.3/hvEdit/bin/xmls/hvEdit-"+fExp+".xml";
  if (!QFile::exists(fln)) { // administrations file not found
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: couldn't find startup administration file (%s), will try some default stuff.", fln.ascii());
  } else {
    PHvAdminXMLParser handler(this);
    QFile xmlFile(fln);
    QXmlInputSource source( &xmlFile );
    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );
    reader.parse( source );
  }

  // check if XML startup file could be read
  if (fNoOdbRoots == 0) {
    fNoOdbRoots = 1;
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Number of ODB HV roots is 0, will try 1.");
  }
  if (!fb_MidasOdbHvRoot) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read ODB HV root path settings, will "
           "try '%s' instead.", fMidasOdbHvRoot->at(0)->ascii());
  }
  if (!fb_MidasOdbHvNames) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read ODB HV names path settings, will "
           "try '%s' instead.", fMidasOdbHvNames.ascii());
  }
  if (!fb_MidasOdbHvDemand) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read ODB HV demand path settings, will "
           "try '%s' instead.", fMidasOdbHvDemand.ascii());
  }
  if (!fb_MidasOdbHvMeasured) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read ODB HV measured path settings, will "
           "try '%s' instead.", fMidasOdbHvMeasured.ascii());
  }
  if (!fb_MidasOdbHvCurrent) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read ODB HV current path settings, will "
           "try '%s' instead.", fMidasOdbHvCurrent.ascii());
  }
  if (!fb_MidasOdbHvCurrentLimit) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read ODB HV demand path settings, will "
           "try '%s' instead.", fMidasOdbHvCurrentLimit.ascii());
  }
  if (!fb_TerminationTimeout) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read termination timeout, will "
           "try '%d' instead.", fTerminationTimeout);
  }
  if (!fb_DemandInVPresent) {
    cm_msg(MINFO, "PHvAdmin", "hvEdit: Warning: Couldn't read 'demand in V' flag, will "
           "try '%d' instead.", fDemandInV);
  }
}

//**********************************************************************
// PHvAdmin destructor
//**********************************************************************
/*!
 * <p>Destructor
 */
PHvAdmin::~PHvAdmin()
{
  if (fMidasOdbHvRoot) {
    delete fMidasOdbHvRoot;
    fMidasOdbHvRoot = 0;
  }
}


//**********************************************************************
// GetHvOdbRoot
//**********************************************************************
/*!
 * <p>returns the ODB path of the HV root with number i
 *
 * \param i number of a given ODB HV root
 */
QString* PHvAdmin::GetHvOdbRoot(uint i)
{
  if (fMidasOdbHvRoot->count() < i)
    return 0;
  else
    return fMidasOdbHvRoot->at(i);
}
