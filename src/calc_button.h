/*
    Calc, a scientific calculator for the X window system using the
    TQt widget libraries, available at no cost at http://www.troll.no

    Copyright (C) 1996 Bernd Johannes Wuebben
                       wuebben@math.cornell.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef _CALC_BUTTON_H
#define _CALC_BUTTON_H

#include <kpushbutton.h>

// The class CalcButton is an overriden TQPushButton. It offers extra
// functionality e.g. labels can be richtext or the accels can be
// shown in the label, but the most important thing is that the button
// may have several modes with corresponding labels. When one switches
// modes, the corresponding label is displayed.


enum ButtonModeFlags {ModeNormal = 0, ModeInverse = 1, ModeHyperbolic = 2};

class TQDomNode;

// Each calc button can be in one of several modes.
// The following class describes label, tooltip etc. for each mode...
class ButtonMode
{
public:
  ButtonMode(void) {};
  ButtonMode(TQString &label, TQString &tooltip, bool is_label_richtext)
    : is_label_richtext(is_label_richtext), tooltip(tooltip)
  {
    if (is_label_richtext)
      this->label = "<qt type=\"page\"><center>" + label + "</center></qt>";
    else
      this->label = label;
  };

  TQString label;
  bool is_label_richtext;
  TQString tooltip;
};


class CalcButton : public TQPushButton
{
TQ_OBJECT
  

public:
 enum ButtonType {
   TypeDigit,
   TypeOperator,
   TypeMemory,
   TypeOther,
   TypeEqual,
   TypeHeader
 };

 CalcButton(TQWidget *parent, const char * name = 0); 
 CalcButton(const TQString &label, TQWidget *parent, const char * name = 0,
	     const TQString &tooltip = TQString());

 void addMode(ButtonModeFlags mode, TQString label, TQString tooltip, bool is_label_richtext = false);
 void setButtonType(ButtonType type);
 ButtonType buttonType() const { return _button_type; }
 static bool keys_borders;
 static TQColor keys_borders_color;

public slots: 
  void slotSetMode(ButtonModeFlags mode, bool flag); 
  void slotSetAccelDisplayMode(bool flag);

protected:
 virtual void drawButton(TQPainter *paint);
 virtual void drawButtonLabel(TQPainter *paint);
 void paintLabel(TQPainter *paint);
 virtual void enterEvent(TQEvent *e);
 virtual void leaveEvent(TQEvent *e);

public:
  void invalidateIconCache() {
      m_cachedPix = TQPixmap();  // invalidate cache
      m_cachedPixSize = -1;
      m_cachedPixColor = 0;
      update();
  }
  void setCustomIcon(const unsigned char* data, unsigned int len) {
      if (m_iconData == data && m_iconLen == len) return;
      m_iconData = data;
      m_iconLen = len;
      invalidateIconCache();
  }
  const unsigned char* iconData() const { return m_iconData; }
  unsigned int iconLen() const { return m_iconLen; }

protected:
 bool _show_accel_mode;
 TQString _label;

 ButtonModeFlags _mode_flags;

 TQMap<ButtonModeFlags, ButtonMode> _mode;
 bool _hovered;
 ButtonType _button_type;
 const unsigned char* m_iconData;
 unsigned int m_iconLen;
 TQPixmap m_cachedPix;
 int m_cachedPixSize;
 unsigned int m_cachedPixColor;
};

class KSquareButton : public CalcButton
{
TQ_OBJECT
  

public:
  KSquareButton(TQWidget *parent, const char * name = 0)
    : CalcButton(parent, name) { }; 
 KSquareButton(const TQString &label, TQWidget *parent, const char * name = 0,
	       const TQString &tooltip = TQString())
   : CalcButton(label, parent, name, tooltip) { };

protected:
 virtual void drawButtonLabel(TQPainter *paint);
 void paintLabel(TQPainter *paint);
};

#endif  // _CALC_BUTTON_H
