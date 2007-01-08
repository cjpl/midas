/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include "PHelpWindow.h"
#include <qstatusbar.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qiconset.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qstylesheet.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qapplication.h>
#include <qcombobox.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qobjectlist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qprinter.h>
#include <qsimplerichtext.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>

#include <ctype.h>

PHelpWindow::PHelpWindow( const QString& home_, const QString& _path,
                        QWidget* parent, const char *name )
    : QMainWindow( parent, name, WDestructiveClose ),
      fPathCombo( 0 ), fSelectedURL()
{
    ReadHistory();
    ReadBookmarks();

    fBrowser = new QTextBrowser( this );

    fBrowser->mimeSourceFactory()->setFilePath( _path );
    fBrowser->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    connect( fBrowser, SIGNAL( textChanged() ),
             this, SLOT( TextChanged() ) );

    setCentralWidget( fBrowser );

    if ( !home_.isEmpty() )
        fBrowser->setSource( home_ );

    connect( fBrowser, SIGNAL( highlighted( const QString&) ),
             statusBar(), SLOT( message( const QString&)) );

    resize( 640,700 );

    QPopupMenu* file = new QPopupMenu( this );
    file->insertItem( tr("&New Window"), this, SLOT( NewWindow() ), CTRL+Key_N );
    file->insertItem( tr("&Open File"), this, SLOT( OpenFile() ), CTRL+Key_O );
    file->insertItem( tr("&Print"), this, SLOT( Print() ), CTRL+Key_P );
    file->insertSeparator();
    file->insertItem( tr("&Close"), this, SLOT( close() ), CTRL+Key_Q );
//as    file->insertItem( tr("E&xit"), qApp, SLOT( closeAllWindows() ), CTRL+Key_X );

    // The same three icons are used twice each.
    QIconSet icon_back( QPixmap(_path+"/images/back.xpm") );
    QIconSet icon_forward( QPixmap(_path+"/images/forward.xpm") );
    QIconSet icon_home( QPixmap(_path+"/images/home.xpm") );

    QPopupMenu* go = new QPopupMenu( this );
    fBackwardId = go->insertItem( icon_back,
                                 tr("&Backward"), fBrowser, SLOT( backward() ),
                                 CTRL+Key_Left );
    fForwardId = go->insertItem( icon_forward,
                                tr("&Forward"), fBrowser, SLOT( forward() ),
                                CTRL+Key_Right );
    go->insertItem( icon_home, tr("&Home"), fBrowser, SLOT( home() ) );

    QPopupMenu* help = new QPopupMenu( this );
    help->insertItem( tr("&About ..."), this, SLOT( About() ) );
    help->insertItem( tr("About &Qt ..."), this, SLOT( AboutQt() ) );

    fHist = new QPopupMenu( this );
    QStringList::Iterator it = fHistory.begin();
    for ( ; it != fHistory.end(); ++it )
        fMHistory[ fHist->insertItem( *it ) ] = *it;
    connect( fHist, SIGNAL( activated( int ) ),
             this, SLOT( HistChosen( int ) ) );

    fBookm = new QPopupMenu( this );
    fBookm->insertItem( tr( "Add Bookmark" ), this, SLOT( AddBookmark() ) );
    fBookm->insertSeparator();

    QStringList::Iterator it2 = fBookmarks.begin();
    for ( ; it2 != fBookmarks.end(); ++it2 )
        fMBookmarks[ fBookm->insertItem( *it2 ) ] = *it2;
    connect( fBookm, SIGNAL( activated( int ) ),
             this, SLOT( BookmChosen( int ) ) );

    menuBar()->insertItem( tr("&File"), file );
    menuBar()->insertItem( tr("&Go"), go );
    menuBar()->insertItem( tr( "History" ), fHist );
    menuBar()->insertItem( tr( "Bookmarks" ), fBookm );
    menuBar()->insertSeparator();
    menuBar()->insertItem( tr("&Help"), help );

    menuBar()->setItemEnabled( fForwardId, FALSE);
    menuBar()->setItemEnabled( fBackwardId, FALSE);
    connect( fBrowser, SIGNAL( backwardAvailable( bool ) ),
             this, SLOT( SetBackwardAvailable( bool ) ) );
    connect( fBrowser, SIGNAL( forwardAvailable( bool ) ),
             this, SLOT( SetForwardAvailable( bool ) ) );


    QToolBar* toolbar = new QToolBar( this );
    addToolBar( toolbar, "Toolbar");
    QToolButton* button;

    button = new QToolButton( icon_back, tr("Backward"), "", fBrowser, SLOT(backward()), toolbar );
    connect( fBrowser, SIGNAL( backwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );
    button = new QToolButton( icon_forward, tr("Forward"), "", fBrowser, SLOT(forward()), toolbar );
    connect( fBrowser, SIGNAL( forwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );
    button = new QToolButton( icon_home, tr("Home"), "", fBrowser, SLOT(home()), toolbar );

    toolbar->addSeparator();

    fPathCombo = new QComboBox( TRUE, toolbar );
    connect( fPathCombo, SIGNAL( activated( const QString & ) ),
             this, SLOT( PathSelected( const QString & ) ) );
    toolbar->setStretchableWidget( fPathCombo );
    setRightJustification( TRUE );
    setDockEnabled( DockLeft, FALSE );
    setDockEnabled( DockRight, FALSE );

    fPathCombo->insertItem( home_ );
    fBrowser->setFocus();

}


void PHelpWindow::SetBackwardAvailable( bool b)
{
    menuBar()->setItemEnabled( fBackwardId, b);
}

void PHelpWindow::SetForwardAvailable( bool b)
{
    menuBar()->setItemEnabled( fForwardId, b);
}


void PHelpWindow::TextChanged()
{
    if ( fBrowser->documentTitle().isNull() )
        setCaption( "Qt Example - Helpviewer - " + fBrowser->context() );
    else
        setCaption( "Qt Example - Helpviewer - " + fBrowser->documentTitle() ) ;

    fSelectedURL = fBrowser->context();

    if ( !fSelectedURL.isEmpty() && fPathCombo ) {
        bool exists = FALSE;
        int i;
        for ( i = 0; i < fPathCombo->count(); ++i ) {
            if ( fPathCombo->text( i ) == fSelectedURL ) {
                exists = TRUE;
                break;
            }
        }
        if ( !exists ) {
            fPathCombo->insertItem( fSelectedURL, 0 );
            fPathCombo->setCurrentItem( 0 );
            fMHistory[ fHist->insertItem( fSelectedURL ) ] = fSelectedURL;
        } else
            fPathCombo->setCurrentItem( i );
        fSelectedURL = QString::null;
    }
}

PHelpWindow::~PHelpWindow()
{
    fHistory.clear();
    QMap<int, QString>::Iterator it = fMHistory.begin();
    for ( ; it != fMHistory.end(); ++it )
        fHistory.append( *it );

    QFile f( QDir::currentDirPath() + "/.history" );
    f.open( IO_WriteOnly );
    QDataStream s( &f );
    s << fHistory;
    f.close();

    fBookmarks.clear();
    QMap<int, QString>::Iterator it2 = fMBookmarks.begin();
    for ( ; it2 != fMBookmarks.end(); ++it2 )
        fBookmarks.append( *it2 );

    QFile f2( QDir::currentDirPath() + "/.bookmarks" );
    f2.open( IO_WriteOnly );
    QDataStream s2( &f2 );
    s2 << fBookmarks;
    f2.close();
}

void PHelpWindow::About()
{
    QMessageBox::about( this, "HelpViewer Example",
                        "<p>This example implements a simple HTML help viewer "
                        "using Qt's rich text capabilities</p>"
                        "<p>It's just about 100 lines of C++ code, so don't expect too much :-)</p>"
                        );
}


void PHelpWindow::AboutQt()
{
    QMessageBox::aboutQt( this, "QBrowser" );
}

void PHelpWindow::OpenFile()
{
#ifndef QT_NO_FILEDIALOG
    QString fn = QFileDialog::getOpenFileName( QString::null, QString::null, this );
    if ( !fn.isEmpty() )
        fBrowser->setSource( fn );
#endif
}

void PHelpWindow::NewWindow()
{
    ( new PHelpWindow(fBrowser->source(), "qbrowser") )->show();
}

void PHelpWindow::Print()
{
#ifndef QT_NO_PRINTER
    QPrinter printer;
    printer.setFullPage(TRUE);
    if ( printer.setup( this ) ) {
        QPainter p( &printer );
        QPaintDeviceMetrics metrics(p.device());
        int dpix = metrics.logicalDpiX();
        int dpiy = metrics.logicalDpiY();
        const int margin = 72; // pt
        QRect body(margin*dpix/72, margin*dpiy/72,
                   metrics.width()-margin*dpix/72*2,
                   metrics.height()-margin*dpiy/72*2 );
        QSimpleRichText richText( fBrowser->text(), QFont(), fBrowser->context(), fBrowser->styleSheet(),
                                  fBrowser->mimeSourceFactory(), body.height() );
        richText.setWidth( &p, body.width() );
        QRect view( body );
        int page = 1;
        do {
            richText.draw( &p, body.left(), body.top(), view, colorGroup() );
            view.moveBy( 0, body.height() );
            p.translate( 0 , -body.height() );
            p.drawText( view.right() - p.fontMetrics().width( QString::number(page) ),
                        view.bottom() + p.fontMetrics().ascent() + 5, QString::number(page) );
            if ( view.top()  >= richText.height() )
                break;
            printer.newPage();
            page++;
        } while (TRUE);
    }
#endif
}

void PHelpWindow::PathSelected( const QString &_path )
{
    fBrowser->setSource( _path );
    QMap<int, QString>::Iterator it = fMHistory.begin();
    bool exists = FALSE;
    for ( ; it != fMHistory.end(); ++it ) {
        if ( *it == _path ) {
            exists = TRUE;
            break;
        }
    }
    if ( !exists )
        fMHistory[ fHist->insertItem( _path ) ] = _path;
}

void PHelpWindow::ReadHistory()
{
    if ( QFile::exists( QDir::currentDirPath() + "/.history" ) ) {
        QFile f( QDir::currentDirPath() + "/.history" );
        f.open( IO_ReadOnly );
        QDataStream s( &f );
        s >> fHistory;
        f.close();
        while ( fHistory.count() > 20 )
            fHistory.remove( fHistory.begin() );
    }
}

void PHelpWindow::ReadBookmarks()
{
    if ( QFile::exists( QDir::currentDirPath() + "/.bookmarks" ) ) {
        QFile f( QDir::currentDirPath() + "/.bookmarks" );
        f.open( IO_ReadOnly );
        QDataStream s( &f );
        s >> fBookmarks;
        f.close();
    }
}

void PHelpWindow::HistChosen( int i )
{
    if ( fMHistory.contains( i ) )
        fBrowser->setSource( fMHistory[ i ] );
}

void PHelpWindow::BookmChosen( int i )
{
    if ( fMBookmarks.contains( i ) )
        fBrowser->setSource( fMBookmarks[ i ] );
}

void PHelpWindow::AddBookmark()
{
    fMBookmarks[ fBookm->insertItem( caption() ) ] = fBrowser->context();
}
