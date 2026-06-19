/*

 Calc 

 Copyright (C) Bernd Johannes Wuebben
               wuebben@math.cornell.edu
	       wuebben@kde.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */


#ifndef _D_CALCDISPLAY_H_
#define _D_CALCDISPLAY_H_

#include <stdlib.h>
#include <tqlabel.h>
#include <tqtimer.h>
#include "knumber.h"
#include "calctype.h"

#if defined HAVE_LONG_DOUBLE && defined HAVE_L_FUNCS
	#define PRINT_FLOAT     "%.*Lf"
	#define PRINT_LONG_BIG  "%.*Lg"
	#define PRINT_LONG      "%Lg"
#else
	#define PRINT_FLOAT     "%.*f"
	#define PRINT_LONG_BIG  "%.*g"
	#define PRINT_LONG      "%g"
#endif

#ifdef HAVE_LONG_LONG
	#define PRINT_OCTAL  "%llo"
	#define PRINT_HEX    "%llX"
#else
	#define PRINT_OCTAL  "%lo"
	#define PRINT_HEX    "%lX"
#endif

#define		NUM_STATUS_TEXT 4

/*
  This class provides a pocket calculator display.  The display has
  implicitely two major modes: One is for editing and one is purely
  for displaying.

  When one uses "setAmount", the given amount is displayed, and the
  amount which was possibly typed in before is lost. At the same time
  this new value can not be modified.
  
  On the other hand, "addNewChar" adds a new digit to the amount that
  is being typed in. If "setAmount" was used before, the display is
  cleared and a new input starts.

  TODO: Check overflows, number of digits and such...
*/

#include <tqframe.h>

enum NumBase {
	NB_BINARY = 2,
	NB_OCTAL = 8,
	NB_DECIMAL = 10,
	NB_HEX = 16
};


class TQtLcdWidget;

class CalcDisplay : public TQFrame
{
TQ_OBJECT
  

public:
	CalcDisplay(TQWidget *parent=0, const char *name=0);
	~CalcDisplay();

protected:
	void  mousePressEvent ( TQMouseEvent *);
	virtual void drawContents(TQPainter *p);

public:
	enum Event {
	  EventReset, // resets display
	  EventClear, // if no _error reset display
	  EventError,
	  EventChangeSign
	};
	bool sendEvent(Event const event);
	void deleteLastDigit(void);
	KNumber const & getAmount(void) const;
	void newCharacter(char const new_char);
	bool setAmount(KNumber const & new_amount);
	NumBase base() const { return _num_base; }
	int setBase(NumBase new_base);
	void setBeep(bool flag);
	void setGroupDigits(bool flag);
	void setFixedPrecision(int precision);
	void setPrecision(int precision);
	void setText(TQString const &string);
	void setWordSize(int ws);
	void setExpression(const TQString &expr);
	void clearExpression();
	void setScientificFormat(bool scientific);
	virtual void setFont(const TQFont &font);
	virtual void setPalette(const TQPalette &p);
	virtual void setPaletteBackgroundColor(const TQColor &c);
	void setSmallTextForeground(const TQColor &color);
	void setDisplayType(int type);
	void setCustomDisplayFont(const TQFont &font);
	void setDisplayBorder(bool has_border, const TQColor &color = TQColor(0, 0, 0));
	TQString text() const;
	bool updateDisplay(void);
	void setStatusText(uint i, const TQString& text);
	virtual TQSize sizeHint() const;
	virtual TQSize minimumSizeHint() const;
protected:
	virtual void resizeEvent(TQResizeEvent *e);
private:
	TQLabel *_history_label;
	TQLabel *_main_label;
	TQtLcdWidget *_history_lcd;
	TQtLcdWidget *_main_lcd;
	int _display_type;
	bool _beep;
	bool _groupdigits;
	int  _button;
	bool _lit;
	TQColor _small_text_color;
	NumBase _num_base;
	int _word_size;
	bool _scientificFormat;
	bool _displayBorder;
	TQColor _displayBorderColor;
	TQFont _custom_font;

	int _precision;
	int _fixed_precision; // "-1" = no fixed_precision

	KNumber _display_amount;
private:
	bool changeSign(void);
	void invertColors(void);

	// only used for input of new numbers
	bool _eestate;
	bool _period;
	bool _neg_sign;
	TQString _str_int;
	TQString _str_int_exp;
	TQString _str_status[NUM_STATUS_TEXT];

	TQTimer* selection_timer;

signals:
	void clicked(void);
	void changedText(TQString const &);

public slots:
	void slotCut(void);
	void slotCopy(void);
	void slotPaste(bool bClipboard=true);

private slots:
	void slotSelectionTimedOut(void);
	void slotDisplaySelected(void);
};

#endif // _CALCDISPLAY_H_
