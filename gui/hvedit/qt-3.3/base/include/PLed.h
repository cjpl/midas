/*************************************************************************

  PLed.h

**************************************************************************

     author               : Andreas Suter, 2004/03/01
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

#ifndef _PLED_H_
#define _PLED_H_

#include <qwidget.h>
#include <qcolor.h>
#include <qtimer.h>

/*!
 * \mainpage
 * <p>The PLed class provides a tri-state LED.
 *
 * <p>The PLed class is a modified version of the class KLed from Joerg Habenicht.
 * KLed is part from the KDE library. The reason to implement a slightly different
 * class are twofolded:
 *   -# I wanted to have a class only relying on qt in order to use it as well with
 *      different platforms.
 *   -# I wanted to have a tri-state LED since sometimes it is useful to have a state
 *      'undefined', e.g. if you have a shutter which might hangs between open and 
 *      close in a undefined state.
 */
class PLed : public QWidget
{

  Q_OBJECT
  Q_ENUMS( EState EShape ELook )
  Q_PROPERTY( EState State READ GetState WRITE SetState )
  Q_PROPERTY( EShape Shape READ GetShape WRITE SetShape )
  Q_PROPERTY( ELook Look READ GetLook WRITE SetLook )
  Q_PROPERTY( QColor Color READ GetColor WRITE SetColor )
  Q_PROPERTY( int fDarkFactor READ GetDarkFactor WRITE SetDarkFactor )

public:

  /*!
   * <p> Status of the light.
   *  - 'On'  results in a bright light
   *  - 'Off' results in a darker light
   *  - 'Undefined' results in a blinking light
   */
  enum EState { On, Off, Undefined };

  //! Shape of the LED.
  enum EShape { Rectangular, Circular };

  /*! 
   * <p> Displays a flat, round or sunken LED. </p>
   * <p> Displaying the LED flat is less time and color consuming, but not so nice to see.</p>
   * <p> The sunken LED itself is (certainly) smaller than the round LED because of the 
   * 3 shading circles and is most time consuming. Makes sense for LED > 15x15 pixels.</p>
   */
  enum ELook  { Flat, Raised, Sunken };

  PLed(QWidget *parent=0, const char *name=0);
  PLed(const QColor &col, QWidget *parent=0, const char *name=0);

  PLed(const QColor& col, const int timeout, PLed::EState state,
       PLed::ELook look, PLed::EShape shape, QWidget *parent=0, const char *name=0);


  ~PLed();

  EState GetState() const;

  EShape GetShape() const;

  QColor GetColor() const;

  ELook  GetLook() const;

  int GetDarkFactor() const;

  void SetState(EState state);

  void SetShape(EShape s);

  void SetColor(const QColor& color);

  void SetDarkFactor(int darkfactor);

  void SetLook(ELook look);

  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

public slots:

  void SetUndefined();

  void SetOn();

  void SetOff();

protected:
  virtual void PaintFlat();
  virtual void PaintRound();
  virtual void PaintSunken();
  virtual void PaintRect();
  virtual void PaintRectFrame(bool raised);

  void paintEvent( QPaintEvent * );

private:
  EState  fLed_state; //!< current state of the LED (On, Off, Undefined)
  QColor  fLed_color; //!< current color of the LED
  ELook   fLed_look;  //!< current look of the LED (Flat, Raised, Sunken)
  EShape  fLed_shape; //!< current shape of the LED (Circular, Rectangular)
  
private slots:
  void ToggleLED();

private:
  class PLedPrivate;
  PLedPrivate *fLedPrivate;
};

#endif // _PLED_H_
