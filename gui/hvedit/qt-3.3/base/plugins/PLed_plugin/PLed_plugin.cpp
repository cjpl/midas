/*************************************************************************

  PLed_plugin.cpp

**************************************************************************

     author               : Andreas Suter, 2004/09/27
     copyright            : (C) 2004 by
     email                : andreas.suter@psi.ch

**************************************************************************/

/*************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

*************************************************************************/

#include "PLed_plugin.h"
#include "PLed.h"

/* XPM */
static const char * led_xpm[] = {
"16 16 32 1",
" 	c None",
".	c #006C00",
"+	c #2FF72F",
"@	c #4FFF4F",
"#	c #ABCFAB",
"$	c #002A00",
"%	c #06C206",
"&	c #C5CBC5",
"*	c #02A602",
"=	c #8E998E",
"-	c #87B887",
";	c #508350",
">	c #D4E3D4",
",	c #049704",
"'	c #E3F2E3",
")	c #F0F4F0",
"!	c #E9ECE9",
"~	c #018101",
"{	c #A2B3A2",
"]	c #364F36",
"^	c #005700",
"/	c #32AD32",
"(	c #06B206",
"_	c #004100",
":	c #1FE11F",
"<	c #001700",
"[	c #11BA11",
"}	c #FBFCFB",
"|	c #F6F8F6",
"1	c #203C20",
"2	c #97D297",
"3	c #74C074",
"     }>##>|     ",
"   }#;~~,,/2|   ",
"  );^~,,****/'  ",
" };^.~,(%%%((/) ",
" {_^.~(:+++:%*3 ",
"|]_^.~[+@@@@:*,'",
"&$_^.~*:+@@@:*,#",
"{$__^.~,[:+:[,~-",
"{$$_^^.~~,**,~~-",
"&<$__^^...~~~~.{",
")1$$$_^^^.....^>",
" =<$$$__^^^^^_; ",
" |]<<$$$_____1! ",
"  !]<<<<$$$$1>  ",
"   |=1<<<<$=!   ",
"     )&==&)     "};

PLed_Plugin::PLed_Plugin()
{
}

QStringList PLed_Plugin::keys() const
{
    QStringList list;
    list << "PLed";
    return list;
}

QWidget* PLed_Plugin::create( const QString &key, QWidget* parent, const char* name )
{
    if ( key == "PLed" )
      return new PLed( parent, name );
    return 0;
}

QString PLed_Plugin::group( const QString& feature ) const
{
    if ( feature == "PLed" )
      return "PSI";
    return QString::null;
}

QIconSet PLed_Plugin::iconSet( const QString& ) const
{
    return QIconSet( QPixmap( led_xpm ) );
}

QString PLed_Plugin::includeFile( const QString& feature ) const
{
    if ( feature == "PLed" )
      return "PLed.h";
    return QString::null;
}

QString PLed_Plugin::toolTip( const QString& feature ) const
{
    if ( feature == "PLed" )
      return "three state LED";
    return QString::null;
}

QString PLed_Plugin::whatsThis( const QString& feature ) const
{
    if ( feature == "PLed" )
      return "A three state LED with 'on', 'off', 'undefined' state";
    return QString::null;
}

bool PLed_Plugin::isContainer( const QString& ) const
{
    return FALSE;
}


Q_EXPORT_PLUGIN( PLed_Plugin )
