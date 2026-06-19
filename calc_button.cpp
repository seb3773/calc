/*
    kCalculator, a simple scientific calculator for KDE

    Copyright (C) 1996-2000 Bernd Johannes Wuebben
                            wuebben@kde.org

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

#include <tqsimplerichtext.h>
#include <tqtooltip.h>
#include <tqpainter.h>
#include <tqimage.h>
#include <tqiconset.h>

#include "tqdom.h"

#include "calc_button.h"
#include "icon_utils.h"
#include <tdeaccelmanager.h>

bool CalcButton::keys_borders = false;
TQColor CalcButton::keys_borders_color = TQColor(0,0,0);


CalcButton::CalcButton(TQWidget * parent, const char * name)
  : TQPushButton(parent, name), _show_accel_mode(false),
    _mode_flags(ModeNormal), _hovered(false), _button_type(TypeOther),
    m_iconData(0), m_iconLen(0), m_cachedPixSize(-1), m_cachedPixColor(0)
{
  setAutoDefault(false);
  TDEAcceleratorManager::setNoAccel(this);
}

CalcButton::CalcButton(const TQString &label, TQWidget * parent,
			 const char * name, const TQString &tooltip)
  : TQPushButton(label, parent, name), _show_accel_mode(false),
    _mode_flags(ModeNormal), _hovered(false), _button_type(TypeOther),
    m_iconData(0), m_iconLen(0), m_cachedPixSize(-1), m_cachedPixColor(0)
{
  setAutoDefault(false);
  TDEAcceleratorManager::setNoAccel(this);
  addMode(ModeNormal, label, tooltip);
}

void CalcButton::setButtonType(ButtonType type)
{
  _button_type = type;
  update();
}

void CalcButton::addMode(ButtonModeFlags mode, TQString label, TQString tooltip, bool is_label_richtext)
{
  if (_mode.contains(mode)) _mode.remove(mode);

  _mode[mode] = ButtonMode(label, tooltip, is_label_richtext);

  // Need to put each button into default mode first
  if(mode == ModeNormal) slotSetMode(ModeNormal, true);
}

void CalcButton::slotSetMode(ButtonModeFlags mode, bool flag)
{
  ButtonModeFlags new_mode;

  if (flag) { // if the specified mode is to be set (i.e. flag = true)
  	new_mode = ButtonModeFlags(_mode_flags | mode);
  } else if (_mode_flags && mode) { // if the specified mode is to be cleared (i.e. flag = false)
  	new_mode = ButtonModeFlags(_mode_flags - mode);
  } else {
  	return; // nothing to do
  }

  if (_mode.contains(new_mode)) {
    // save accel, because setting label erases accel
    TQKeySequence _accel = accel();

    if(_mode[new_mode].is_label_richtext)
      _label = _mode[new_mode].label;
    else
      setText(_mode[new_mode].label);
	TQToolTip::remove(this);
    // TQToolTip::add(this, _mode[new_mode].tooltip); // User wants no tooltips
    _mode_flags = new_mode;

    // restore accel
    setAccel(_accel);
  }

  // this is necessary for people pressing CTRL and changing mode at
  // the same time...
  if (_show_accel_mode) slotSetAccelDisplayMode(true);
  
  update();
}

static TQString escape(TQString str)
{
  str.replace('&', "&&");
  return str;
}


void CalcButton::slotSetAccelDisplayMode(bool flag)
{
  _show_accel_mode = flag;

  // save accel, because setting label erases accel
  TQKeySequence _accel = accel();
  
  if (flag == true) {
    setText(escape(TQString(accel())));
  } else {
    setText(_mode[_mode_flags].label);
  }

  // restore accel
  setAccel(_accel);
}

void CalcButton::paintLabel(TQPainter *paint)
{
  if (_mode[_mode_flags].is_label_richtext) {
    TQSimpleRichText _text(_label, font());
    _text.draw(paint, width()/2-_text.width()/2, 0, childrenRegion(), colorGroup());
  } else {
    if (m_iconData && m_iconLen > 0) {
      int p_size;
      if (_button_type == TypeOperator || _button_type == TypeEqual) p_size = height() * 0.40;
      else if (_button_type == TypeDigit) p_size = height() * 0.45;
      else if (_button_type == TypeMemory) p_size = height() * 0.30;
      else p_size = height() * 0.70;

      TQColor fgColor = colorGroup().foreground();
      bool isKey = (_button_type != TypeHeader);
      TQColor customColor = isKey ? fgColor : TQColor();
      unsigned int colorKey = customColor.isValid() ? customColor.rgb() : 0;

      if (p_size != m_cachedPixSize || colorKey != m_cachedPixColor || m_cachedPix.isNull()) {
        m_cachedPix = IconUtils::load(m_iconData, m_iconLen, p_size, p_size, customColor);
        m_cachedPixSize = p_size;
        m_cachedPixColor = colorKey;
      }

      int x = (width() - m_cachedPix.width()) / 2;
      int y = (height() - m_cachedPix.height()) / 2;
      if (isEnabled()) {
        paint->drawPixmap(x, y, m_cachedPix);
      } else {
        TQIconSet icon(m_cachedPix);
        paint->drawPixmap(x, y, icon.pixmap(TQIconSet::Automatic, TQIconSet::Disabled));
      }
    } else {
      TQFont f = font();
      int p_size;
      if (_button_type == TypeOperator || _button_type == TypeEqual) p_size = height() * 0.40;
      else if (_button_type == TypeDigit) {
        p_size = height() * 0.45;
        f.setBold(true);
      }
      else if (_button_type == TypeMemory || _button_type == TypeOther) p_size = height() * 0.45;
      else p_size = height() * 0.35;
      
      if (p_size > 0) f.setPixelSize(p_size);
      paint->setFont(f);
      paint->drawText(rect(), TQt::AlignCenter, text());
    }
  }
}

void CalcButton::drawButtonLabel(TQPainter *paint)
{
  if (_show_accel_mode) {
    TQPushButton::drawButtonLabel(paint);
  } else if (_mode.contains(_mode_flags)) {
    paintLabel(paint);
  }
}


void KSquareButton::paintLabel(TQPainter *paint)
{
  if (!(_mode_flags & ModeInverse) && m_iconData && m_iconLen > 0) {
    CalcButton::paintLabel(paint);
    return;
  }

  int w = width();
  int w2 = w/2 - 13;
  int h = height();
  int h2 = h/2 - 7;
  // in some KDE-styles (.NET, Phase,...) we have to set the painter back to the right color
  paint->setPen(foregroundColor());
  // these statements are for the improved
  // representation of the sqrt function
  paint->drawLine(w2, 11 + h2, w2 + 2, 7 + h2);
  paint->drawLine(w2 + 2, 7 + h2, w2 + 4, 14 + h2);
  paint->drawLine(w2 + 4, 14 + h2, w2 + 6, 1 + h2);
  paint->drawLine(w2 + 6, 1 + h2, w2 + 27, 1 + h2);
  paint->drawLine(w2 + 27, 1 + h2, w2 + 27, 4 + h2);
  // add a three for the cube root
  if (_mode_flags & ModeInverse) {
    paint->drawText(w2-2, 9 + h2, TQString("³"));
  }
}

void KSquareButton::drawButtonLabel(TQPainter *paint)
{
  if (_show_accel_mode) {
    TQPushButton::drawButtonLabel(paint);
  } else if (_mode.contains(_mode_flags)) {
    paintLabel(paint);
  }
}

void CalcButton::enterEvent(TQEvent *e)
{
  _hovered = true;
  update();
  TQPushButton::enterEvent(e);
}

void CalcButton::leaveEvent(TQEvent *e)
{
  _hovered = false;
  update();
  TQPushButton::leaveEvent(e);
}

void CalcButton::drawButton(TQPainter *paint)
{
  bool enabled = isEnabled();
  bool down = isDown() || (isToggleButton() && isOn());
  TQColor fill_color;

  if (isFlat() && !_hovered && !down) {
    paint->fillRect(rect(), palette().color(TQPalette::Active, TQColorGroup::Background));
    TQColor text_color;
    if (enabled) {
      text_color = palette().color(TQPalette::Active, TQColorGroup::ButtonText);
    } else {
      TQColor bg = palette().color(TQPalette::Active, TQColorGroup::Background);
      if (bg.red() + bg.green() + bg.blue() < 300) {
        text_color = TQColor(90, 90, 90);
      } else {
        text_color = TQColor(150, 150, 150);
      }
    }
    paint->setPen(text_color);
    drawButtonLabel(paint);
    if (hasFocus()) {
      TQRect r = rect();
      paint->setPen(TQPen(TQt::black, 1, TQt::DotLine));
      paint->drawRect(r.x() + 2, r.y() + 2, r.width() - 4, r.height() - 4);
    }
    return;
  }

  TQColor base_color = palette().color(TQPalette::Active, TQColorGroup::Button);
  int val = (base_color.red() * 299 + base_color.green() * 587 + base_color.blue() * 114) / 1000;
  if (!enabled) {
    fill_color = base_color;
  } else if (down) {
    fill_color = base_color.dark(115);
  } else if (_hovered) {
    if (val > 128) {
      fill_color = base_color.dark(108);
    } else {
      int r = base_color.red();
      int g = base_color.green();
      int b = base_color.blue();
      int nr = r * 1.25;
      int ng = g * 1.25;
      int nb = b * 1.25;
      if (nr < r + 30) nr = r + 30;
      if (ng < g + 30) ng = g + 30;
      if (nb < b + 30) nb = b + 30;
      if (nr > 255) nr = 255;
      if (ng > 255) ng = 255;
      if (nb > 255) nb = 255;
      fill_color = TQColor(nr, ng, nb);
    }
  } else {
    fill_color = base_color;
  }

  // Draw background
  paint->fillRect(rect(), fill_color);

  // Draw border if keys_borders is enabled
  if (keys_borders && _button_type != TypeHeader) {
    paint->setPen(keys_borders_color);
    paint->drawRect(0, 0, width() - 1, height() - 1);
  }

  // Set the text color and draw label
  TQColor text_color;
  if (enabled) {
    text_color = palette().color(TQPalette::Active, TQColorGroup::ButtonText);
  } else {
    TQColor bg = palette().color(TQPalette::Active, TQColorGroup::Button);
    if (bg.red() + bg.green() + bg.blue() < 300) {
      text_color = TQColor(90, 90, 90);
    } else {
      text_color = TQColor(150, 150, 150);
    }
  }
  paint->setPen(text_color);

  // Dynamically calculate font size based on button height to scale perfectly with window resizes
  TQFont orig_font = paint->font();
  TQFont f = orig_font;
  
  int font_size;
  if (_button_type == TypeDigit) {
    font_size = height() * 0.45; // Digits are very large in Windows Calc
    f.setBold(true);
  } else if (_button_type == TypeOperator || _button_type == TypeEqual) {
    font_size = height() * 0.40;
  } else if (_button_type == TypeMemory) {
    font_size = height() * 0.30;
  } else {
    font_size = height() * 0.35;
  }
  
  if (font_size < 8) font_size = 8;
  f.setPixelSize(font_size);
  
  paint->setFont(f);

  drawButtonLabel(paint);

  // Restore font
  paint->setFont(orig_font);
}

#include "calc_button.moc"

