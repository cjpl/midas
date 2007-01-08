/*******************************************************************************************

  PPwd.h

  implementation of the virtual class 'PPwdBase' from the Qt designer
  (see 'PPwdBase.ui').

  For details see the implementation: PPwd.cpp

*********************************************************************************************

    begin                : Andreas Suter, 2002/11/25
    modfied:             :
    copyright            : (C) 2003 by
    email                : andreas.suter@psi.ch

  $Log$
  Revision 1.1  2006/01/31 15:01:28  nemu
  replaces cmPwd.h

  Revision 1.1  2004/09/30 09:19:16  suter_a
  newly added



********************************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PPWD_H_
#define _PPWD_H_

#include "PPwdBase.h"

class PPwd : public PPwdBase
{
  Q_OBJECT

  // Constructor
  public:
    PPwd(char *password, QWidget *parent = 0, const char *name = 0, bool modal = FALSE,
         WFlags f = WDestructiveClose);
    ~PPwd();

  // Attributes
  private:
    char *fPwd;

  // Slots
  public slots:
    void GetPassword();
};

#endif // _PPWD_H_
