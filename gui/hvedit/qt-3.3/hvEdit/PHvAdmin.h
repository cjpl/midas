/********************************************************************************************

  PHvAdmin.h

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

#ifndef _PHVADMIN_H_
#define _PHVADMIN_H_

#include <qstring.h>
#include <qptrlist.h>
#include <qxml.h>

class PHvAdmin;

//-----------------------------------------------------------------------------
class PHvAdminXMLParser : public QXmlDefaultHandler
{
  public:
    PHvAdminXMLParser(PHvAdmin*);

  // operations
  private:
    enum EHvAdminWords {empty, defaultSettingDir, defaultDocDir, 
                        terminationTimeout, demandInV,
                        hvRoot, hvNames,
                        hvDemand, hvMeasured, hvCurrent, hvCurrentLimit};

    bool startDocument();
    bool startElement( const QString&, const QString&, const QString& ,
                       const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );

    bool characters(const QString&);
    bool endDocument();

    EHvAdminWords  fKey;
    PHvAdmin      *fHva;
    int            fOdbRootCounter;
};

//-----------------------------------------------------------------------------
class PHvAdmin
{
  public:
    PHvAdmin(char *host, char *exp);
    ~PHvAdmin();

  // attributes
  private:
    QString fHost;
    QString fExp;
    QPtrList<QString> *fMidasOdbHvRoot;
    QString fDefaultDirHvSettings;
    QString fDefaultDirDocu;
    int     fTerminationTimeout;
    bool    fDemandInV;
    QString fMidasOdbHvNames;
    QString fMidasOdbHvDemand;
    QString fMidasOdbHvMeasured;
    QString fMidasOdbHvCurrent;
    QString fMidasOdbHvCurrentLimit;
    bool    fb_DefaultDirHvSettings;
    bool    fb_DefaultDirDocu;
    bool    fb_TerminationTimeout;
    bool    fb_DemandInVPresent;
    bool    fb_MidasOdbHvRoot;
    bool    fb_MidasOdbHvNames;
    bool    fb_MidasOdbHvDemand;
    bool    fb_MidasOdbHvMeasured;
    bool    fb_MidasOdbHvCurrent;
    bool    fb_MidasOdbHvCurrentLimit;

  private:
    friend class PHvAdminXMLParser;

  // operations
  public:
    int      fNoOdbRoots;
    QString *GetHostName() { return &fHost; }
    QString *GetExpName() { return &fExp; }
    QString *GetHvOdbRoot(uint i);
    QString *GetHvDefaultDir() { return &fDefaultDirHvSettings; }
    QString *GetHvDefaultDoc() { return &fDefaultDirDocu; }
    int      GetTerminationTimeout() { return fTerminationTimeout; }
    bool     GetDemandInV() { return fDemandInV; }
    QString *GetHvOdbNames() { return &fMidasOdbHvNames; }
    QString *GetHvOdbDemand() { return &fMidasOdbHvDemand; }
    QString *GetHvOdbMeasured() { return &fMidasOdbHvMeasured; }
    QString *GetHvOdbCurrent() { return &fMidasOdbHvCurrent; }
    QString *GetHvOdbCurrentLimit() { return &fMidasOdbHvCurrentLimit; }
};

#endif // _PHVADMIN_H_
