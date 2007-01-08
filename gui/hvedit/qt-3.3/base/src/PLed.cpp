/*************************************************************************

  PLed.cpp

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

#include <qpainter.h>
#include <qcolor.h>

#include "PLed.h"

//**********************************************************************************
/*!
 * Private class to hide some of the technical handling of the LED widget
 */
class PLed::PLedPrivate
{
  friend class PLed;

  int fDark_factor; //!< darkfactor
  QColor fOffcolor; //!< the Off color
  QColor fOldcolor; //!< the old color (used for Circular, Undefined)
  int fRectCount;   //!< Rectangular toggle switch (used for Undefined)

  int fTimeout;     //!< time out for the cyclic timer (used for Undefined)
  QTimer *fTimer;   //!< timer, used as a cyclic timer (used for Undefined)
};


//**********************************************************************************
/*!
 * <p>Constructs a green, round LED widget which will initially be turned on.</p>
 *
 * \param parent is a pointer to the parent widget.
 * \param name is an internal name
 */
PLed::PLed(QWidget *parent, const char *name)
  : QWidget( parent, name),
    fLed_state(On),
    fLed_look(Raised),
    fLed_shape(Circular)
{
  QColor col(green);
  fLedPrivate = new PLed::PLedPrivate;
  fLedPrivate->fDark_factor = 300;
  fLedPrivate->fOffcolor = col.dark(300);
  fLedPrivate->fOldcolor = col;
  fLedPrivate->fRectCount = 0;

  fLedPrivate->fTimeout = 500;
  fLedPrivate->fTimer = new QTimer( this );
  connect( fLedPrivate->fTimer, SIGNAL(timeout()), this, SLOT(ToggleLED()) );

  SetColor(col);
}


//**********************************************************************************
/*!
 * <p>Constructs round LED widget with the color 'col', which will initially be turned on.</p>
 *
 * \param col initial color of the LED.
 * \param parent is a pointer to the parent widget.
 * \param name is an internal name.
 */
PLed::PLed(const QColor& col, QWidget *parent, const char *name)
  : QWidget( parent, name),
    fLed_state(On),
    fLed_look(Raised),
    fLed_shape(Circular)
{
  fLedPrivate = new PLed::PLedPrivate;
  fLedPrivate->fDark_factor = 300;
  fLedPrivate->fOffcolor = col.dark(300);
  fLedPrivate->fOldcolor = col;
  fLedPrivate->fRectCount = 0;

  fLedPrivate->fTimeout = 500;
  fLedPrivate->fTimer = new QTimer( this );
  connect( fLedPrivate->fTimer, SIGNAL(timeout()), this, SLOT(ToggleLED()) );

  SetColor(col);
}

//**********************************************************************************
/*!
 * <p>Constructor with the color, timeout, state, look, shape, the parent widget, 
 * and the name.</p>
 *
 * <p>Differs from above only in the parameters, which configure all settings.</p>
 *
 * \param col initial color of the LED.
 * \param timeout is the switching time in (ms) for the state 'Undefined'. Default
 *        is 500 (ms).
 * \param state initial state of the LED ('On', 'Off', 'Undefined')
 * \param look initial look of the LED ('Flat', 'Raised', 'Sunken')
 * \param shape initial shape of the LED ('Circular', 'Rectangular')
 * \param parent is a pointer to the parent widget.
 * \param name is an internal name.
 */
PLed::PLed(const QColor& col, const int timeout, PLed::EState state,
       PLed::ELook look, PLed::EShape shape, QWidget *parent, const char *name )
  : QWidget(parent, name),
    fLed_state(state),
    fLed_look(look),
    fLed_shape(shape)
{
  fLedPrivate = new PLed::PLedPrivate;
  fLedPrivate->fDark_factor = 300;
  fLedPrivate->fOffcolor = col.dark(300);
  fLedPrivate->fOldcolor = col;
  fLedPrivate->fRectCount = 0;

  fLedPrivate->fTimeout = timeout;
  fLedPrivate->fTimer = new QTimer( this );
  connect( fLedPrivate->fTimer, SIGNAL(timeout()), this, SLOT(ToggleLED()) );
  if (state == Undefined) {
    if (!fLedPrivate->fTimer->isActive()) {
      fLedPrivate->fTimer->start(fLedPrivate->fTimeout, FALSE); // start cyclic timer
    }
  }

  SetColor(col);
}

//**********************************************************************************
//! Destructor.
PLed::~PLed()
{
  delete fLedPrivate;
}

//**********************************************************************************
/*!
 * paint event handler.
 *
 * <p><b>return:</b> void</p>
 */
void PLed::paintEvent(QPaintEvent *)
{
  switch(fLed_shape) {
    case Rectangular:
      switch (fLed_look) {
        case Sunken:
          PaintRectFrame(false);
          break;
        case Raised:
          PaintRectFrame(true);
          break;
        case Flat:
          PaintRect();
          break;
        default:
          qWarning("in class PLed: no PLed::Look set");
      }
      break;
    case Circular:
      switch (fLed_look) {
        case Flat:
          PaintFlat();
          break;
        case Raised:
          PaintRound();
          break;
        case Sunken:
          PaintSunken();
          break;
        default:
          qWarning("in class PLed: no PLed::Look set");
      }
      break;
    default:
      qWarning("in class PLed: no PLed::Shape set");
      break;
  }
}

//**********************************************************************************
/*!
 * paints a round flat LED.
 *
 * <p><b>return:</b> void</p>
 */
void PLed::PaintFlat() // paint a ROUND FLAT led lamp
{
    QPainter paint;
    QColor   color;
    QBrush   brush;
    QPen     pen;

    // Initialize coordinates, width, and height of the LED
    //
    int width = this->width();
    // Make sure the LED is round!
    if (width > this->height())
        width = this->height();
    width -= 2; // leave one pixel border
    if (width < 0)
      width = 0;


    // start painting widget
    //
    paint.begin( this );

    // Set the color of the LED according to given parameters
    if (fLed_state != Undefined) { // On, Off
      color = ( !fLed_state ) ? fLed_color : fLedPrivate->fOffcolor;
    } else { // Undefined, i.e. toggle colors
      if (fLedPrivate->fOldcolor != fLed_color)
        color = fLed_color;
      else
        color = fLedPrivate->fOffcolor;
      fLedPrivate->fOldcolor = color;
    }

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn with a thin gray "border" (pen)
    brush.setStyle( QBrush::SolidPattern );
    brush.setColor( color );

    pen.setWidth( 1 );
    color = colorGroup().dark();
    pen.setColor( color );          // Set the pen accordingly

    paint.setPen( pen );            // Select pen for drawing
    paint.setBrush( brush );        // Assign the brush to the painter

    // Draws a "flat" LED with the given color:
    paint.drawEllipse( 1, 1, width, width );

    paint.end();
    //
    // painting done
}

//**********************************************************************************
/*!
 * paints a round raised LED.
 *
 * <p><b>return:</b> void</p>
 */
void PLed::PaintRound() // paint a ROUND RAISED led lamp
{
    QPainter paint;
    QColor   color;
    QBrush   brush;
    QPen     pen;

    // Initialize coordinates, width, and height of the LED
    int width = this->width();

    // Make sure the LED is round!
    if (width > this->height())
      width = this->height();
    width -= 2; // leave one pixel border
    if (width < 0)
      width = 0;

    // start painting widget
    //
    paint.begin( this );

    // Set the color of the LED according to given parameters
    if (fLed_state != Undefined) { // On, Off
      color = ( !fLed_state ) ? fLed_color : fLedPrivate->fOffcolor;
    } else { // Undefined, i.e. toggle colors
      if (fLedPrivate->fOldcolor != fLed_color)
        color = fLed_color;
      else
        color = fLedPrivate->fOffcolor;
      fLedPrivate->fOldcolor = color;
    }

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    brush.setStyle( QBrush::SolidPattern );
    brush.setColor( color );
    paint.setBrush( brush );        // Assign the brush to the painter

    // Draws a "flat" LED with the given color:
    paint.drawEllipse( 1, 1, width, width );

    // Draw the bright light spot of the LED now, using modified "old"
    // painter routine taken from KDEUIs PLed widget:

    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth( 2 );

    // shrink the light on the LED to a size about 2/3 of the complete LED
    int pos = width/5 + 1;
    int light_width = width;
    light_width *= 2;
    light_width /= 3;

    // Calculate the LEDs "light factor":
    int light_quote = (130*2/(light_width?light_width:1))+100;

    // Now draw the bright spot on the LED:
    while (light_width) {
      color = color.light( light_quote ); // make color lighter
      pen.setColor( color );              // set color as pen color
      paint.setPen( pen );                // select the pen for drawing
      paint.drawEllipse( pos, pos, light_width, light_width );    // draw the ellipse (circle)
      light_width--;
      if (!light_width)
        break;
      paint.drawEllipse( pos, pos, light_width, light_width );
      light_width--;
      if (!light_width)
        break;
      paint.drawEllipse( pos, pos, light_width, light_width );
      pos++; light_width--;
    } // end while

    // Drawing of bright spot finished, now draw a thin gray border
    // around the LED; it looks nicer that way. We do this here to
    // avoid that the border can be erased by the bright spot of the LED

    pen.setWidth( 1 );
    color = colorGroup().dark();
    pen.setColor( color );          // Set the pen accordingly
    paint.setPen( pen );            // Select pen for drawing
    brush.setStyle( QBrush::NoBrush );      // Switch off the brush
    paint.setBrush( brush );            // This avoids filling of the ellipse

    paint.drawEllipse( 1, 1, width, width );

    paint.end();
    //
    // painting done
}

//**********************************************************************************
/*!
 * paints a round sunken LED.
 *
 * <p><b>return:</b> void</p>
 */
void PLed::PaintSunken() // paint a ROUND SUNKEN led lamp
{
    QPainter paint;
    QColor   color;
    QBrush   brush;
    QPen     pen;

    // First of all we want to know what area should be updated
    // Initialize coordinates, width, and height of the LED
    int width = this->width();

    // Make sure the LED is round!
    if (width > this->height())
      width = this->height();
    width -= 2; // leave one pixel border
    if (width < 0)
      width = 0;

    // maybe we could stop HERE, if width <=0 ?

    // start painting widget
    //
    paint.begin( this );

    // Set the color of the LED according to given parameters
    if (fLed_state != Undefined) { // On, Off
      color = ( !fLed_state ) ? fLed_color : fLedPrivate->fOffcolor;
    } else { // Undefined, i.e. toggle colors
      if (fLedPrivate->fOldcolor != fLed_color)
        color = fLed_color;
      else
        color = fLedPrivate->fOffcolor;
      fLedPrivate->fOldcolor = color;
    }

    // Set the brush to SolidPattern, this fills the entire area
    // of the ellipse which is drawn first
    brush.setStyle( QBrush::SolidPattern );
    brush.setColor( color );
    paint.setBrush( brush );                // Assign the brush to the painter

    // Draws a "flat" LED with the given color:
    paint.drawEllipse( 1, 1, width, width );

    // Draw the bright light spot of the LED now, using modified "old"
    // painter routine taken from KDEUIs PLed widget:

    // Setting the new width of the pen is essential to avoid "pixelized"
    // shadow like it can be observed with the old LED code
    pen.setWidth( 2 );

    // shrink the light on the LED to a size about 2/3 of the complete LED
    int pos = width/5 + 1;
    int light_width = width;
    light_width *= 2;
    light_width /= 3;

    // Calculate the LEDs "light factor":
    int light_quote = (130*2/(light_width?light_width:1))+100;

    // Now draw the bright spot on the LED:
    while (light_width) {
      color = color.light( light_quote );                      // make color lighter
      pen.setColor( color );                                   // set color as pen color
      paint.setPen( pen );                                     // select the pen for drawing
      paint.drawEllipse( pos, pos, light_width, light_width ); // draw the ellipse (circle)
      light_width--;
      if (!light_width)
        break;
      paint.drawEllipse( pos, pos, light_width, light_width );
      light_width--;
      if (!light_width)
        break;
      paint.drawEllipse( pos, pos, light_width, light_width );
      pos++; light_width--;
    } // end while

    // Drawing of bright spot finished, now draw a thin border
    // around the LED which resembles a shadow with light coming
    // from the upper left.

    pen.setWidth( 3 ); // ### shouldn't this value be smaller for smaller LEDs?
    brush.setStyle( QBrush::NoBrush );              // Switch off the brush
    paint.setBrush( brush );                        // This avoids filling of the ellipse

    // Set the initial color value to colorGroup().light() (bright) and start
    // drawing the shadow border at 45 (45*16 = 720).

    int angle = -720;
    color = colorGroup().light();

    for ( int arc = 120; arc < 2880; arc += 240 ) {
      pen.setColor( color );
      paint.setPen( pen );
      paint.drawArc( 1, 1, width, width, angle + arc, 240 );
      paint.drawArc( 1, 1, width, width, angle - arc, 240 );
      color = color.dark( 110 ); //FIXME: this should somehow use the contrast value
    }   // end for ( angle = 720; angle < 6480; angle += 160 )

    paint.end();
    //
    // painting done
}

//**********************************************************************************
/*!
 * paints a rectangular flat LED
 *
 * <p><b>return:</b> void</p>
 */
void PLed::PaintRect()
{
  QPainter painter(this);
  QBrush lightBrush(fLed_color);
  QBrush darkBrush(fLedPrivate->fOffcolor);
  QPen pen(fLed_color.dark(300));
  int w=width();
  int h=height();
  // -----
  switch(fLed_state) {
    case On:
      painter.setBrush(lightBrush);
      painter.drawRect(0, 0, w, h);
      break;
    case Off:
      painter.setBrush(darkBrush);
      painter.drawRect(0, 0, w, h);
      painter.setPen(pen);
      painter.drawLine(0, 0, w, 0);
      painter.drawLine(0, h-1, w, h-1);
      // Draw verticals
      int i;
      for(i=0; i < w; i+= 4 /* dx */)
        painter.drawLine(i, 1, i, h-1);
      break;
    case Undefined:
      if (fLedPrivate->fRectCount) {
        painter.setBrush(lightBrush);
        painter.drawRect(0, 0, w, h);
      } else {
        painter.setBrush(darkBrush);
        painter.drawRect(0, 0, w, h);
        painter.setPen(pen);
        painter.drawLine(0, 0, w, 0);
        painter.drawLine(0, h-1, w, h-1);
        // Draw verticals
        int i;
        for(i=0; i < w; i+= 4 /* dx */)
          painter.drawLine(i, 1, i, h-1);
      }  
      break;
    default: break;
  }
}

//**********************************************************************************
/*!
 * paints a rectangular raised or sunken LED
 *
 * <p><b>return:</b> void</p>
 *
 * \param raised if true -> 'Raised' otherwise 'Sunken'
 */
void PLed::PaintRectFrame(bool raised)
{
  QPainter painter(this);
  QBrush lightBrush(fLed_color);
  QBrush darkBrush(fLedPrivate->fOffcolor);
  int w=width();
  int h=height();
  QColor black=Qt::black;
  QColor white=Qt::white;
  // -----
  if (raised) {
    painter.setPen(white);
    painter.drawLine(0, 0, 0, h-1);
    painter.drawLine(1, 0, w-1, 0);
    painter.setPen(black);
    painter.drawLine(1, h-1, w-1, h-1);
    painter.drawLine(w-1, 1, w-1, h-1);
    switch (fLed_state) {
      case On:
        painter.fillRect(1, 1, w-2, h-2, lightBrush);
        break;
      case Off:
        painter.fillRect(1, 1, w-2, h-2, darkBrush);
        break;
      case Undefined:
        if (fLedPrivate->fRectCount)
          painter.fillRect(1, 1, w-2, h-2, lightBrush);
        else
          painter.fillRect(1, 1, w-2, h-2, darkBrush);
        break;
      default:
        break;  
    }
  } else {
    painter.setPen(black);
    painter.drawRect(0,0,w,h);
    painter.drawRect(0,0,w-1,h-1);
    painter.setPen(white);
    painter.drawRect(1,1,w-1,h-1);
    switch (fLed_state) {
      case On:
        painter.fillRect(2, 2, w-4, h-4, lightBrush);
        break;
      case Off:
        painter.fillRect(2, 2, w-4, h-4, darkBrush);
        break;
      case Undefined:
        if (fLedPrivate->fRectCount) // toggle switch
          painter.fillRect(2, 2, w-4, h-4, lightBrush);
        else
          painter.fillRect(2, 2, w-4, h-4, darkBrush);
        break;
      default:
        break;  
    }
  }
}

//**********************************************************************************
//! Returns the current state of the widget (On/Off/Undefined).
PLed::EState PLed::GetState() const
{
  return fLed_state;
}

//**********************************************************************************
//! Returns the current shape of the widget (Circular/Rectangular).
PLed::EShape PLed::GetShape() const
{
  return fLed_shape;
}

//**********************************************************************************
//! Returns the current color of the widget.
QColor PLed::GetColor() const
{
  return fLed_color;
}

//! Returns the current look of the widget (Flat/Sunken/Raised).
PLed::ELook PLed::GetLook() const
{
  return fLed_look;
}

//**********************************************************************************
/*!
 * Sets the state of the widget to On, Off or Undefined.
 *
 * <p><b>return:</b> void</p>
 *
 * \param state of the widget
 */
void PLed::SetState( EState state )
{
  if (fLed_state != state) {
    if (state == Undefined) {
      if (!fLedPrivate->fTimer->isActive())
        fLedPrivate->fTimer->start(fLedPrivate->fTimeout, FALSE);
    } else {
      if (fLedPrivate->fTimer->isActive())
        fLedPrivate->fTimer->stop();
    }
    fLed_state = state;
    update();
  }
}

//**********************************************************************************
/*!
 * Sets the shape of the widget to Flat, Sunken or Raised.
 *
 * <p><b>return:</b> void</p>
 *
 * \param s shape of the widget
 */
void PLed::SetShape(PLed::EShape s)
{
  if(fLed_shape!=s) {
    fLed_shape = s;
    update();
  }
}

//**********************************************************************************
/*!
 * Sets the color of the widget.
 *
 * <p><b>return:</b> void</p>
 *
 * \param col color of the widget
 */
void PLed::SetColor(const QColor& col)
{
  if(fLed_color!=col) {
    fLed_color = col;
    fLedPrivate->fOffcolor = col.dark(fLedPrivate->fDark_factor);
    update();
  }
}

//**********************************************************************************
/*!
 * Sets the dark factor of the widget. This is needed for the
 * Off state and the toggle color of the Undefined state.
 *
 * <p><b>return:</b> void</p>
 *
 * \param darkfactor numbers > 100 make the color darker, e.g. 300 
 *        results in a color with 1/3 of the original one. 
 */
void PLed::SetDarkFactor(int darkfactor)
{
  if (fLedPrivate->fDark_factor != darkfactor) {
    fLedPrivate->fDark_factor = darkfactor;
    fLedPrivate->fOffcolor = fLed_color.dark(darkfactor);
    update();
  }
}

//**********************************************************************************
//! returns the darkfactor of the LED
int PLed::GetDarkFactor() const
{
  return fLedPrivate->fDark_factor;
}

//**********************************************************************************
/*!
 * Sets the look of the widget to Flat, Sunken or Raised.
 *
 * <p><b>return:</b> void</p>
 *
 * \param look of the widget
 */
void PLed::SetLook( ELook look )
{
  if(fLed_look!=look) {
    fLed_look = look;
    update();
  }
}

//**********************************************************************************
//! sets the LED to the state Undefined
void PLed::SetUndefined()
{
  SetState(Undefined);
}

//**********************************************************************************
//! sets the LED to the state On
void PLed::SetOn()
{
  SetState(On);
}

//**********************************************************************************
//! sets the LED to the state Off
void PLed::SetOff()
{
  SetState(Off);
}

//**********************************************************************************
//! called by time out in the state Undefined. Toggles the LED
void PLed::ToggleLED()
{
  ++fLedPrivate->fRectCount %= 2; // toggler for shape rectangular
  update();            // update widget
}

//**********************************************************************************
//! returns a size hint for the widget
QSize PLed::sizeHint() const
{
  return QSize(16, 16);
}

//**********************************************************************************
//! returns a minimum size hint for the widget
QSize PLed::minimumSizeHint() const
{
  return QSize(16, 16 );
}

// end
