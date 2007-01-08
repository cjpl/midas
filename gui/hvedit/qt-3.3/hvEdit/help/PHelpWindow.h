/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef _PHELPWINDOW_H_
#define _PHELPWINDOW_H_

#include <qmainwindow.h>
#include <qtextbrowser.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdir.h>

class QComboBox;
class QPopupMenu;

class PHelpWindow : public QMainWindow
{
    Q_OBJECT
public:
    PHelpWindow( const QString& home_,  const QString& path, QWidget* parent = 0, const char *name=0 );
    ~PHelpWindow();

private slots:
    void SetBackwardAvailable( bool );
    void SetForwardAvailable( bool );

    void TextChanged();
    void About();
    void AboutQt();
    void OpenFile();
    void NewWindow();
    void Print();

    void PathSelected( const QString & );
    void HistChosen( int );
    void BookmChosen( int );
    void AddBookmark();

private:
    void ReadHistory();
    void ReadBookmarks();

    QTextBrowser* fBrowser;
    QComboBox *fPathCombo;
    int fBackwardId, fForwardId;
    QString fSelectedURL;
    QStringList fHistory, fBookmarks;
    QMap<int, QString> fMHistory, fMBookmarks;
    QPopupMenu *fHist, *fBookm;

};

#endif // _PHELPWINDOW_H_

