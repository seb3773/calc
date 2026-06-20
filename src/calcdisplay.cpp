/*
    $Id$

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

#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include <tqclipboard.h>
#include <tqpainter.h>
#include <tqregexp.h>
#include <tqpopupmenu.h>
#include <tqimage.h>
#include "embedded_icons.h"
#include "icon_utils.h"

#include <tdeglobal.h>
#include <tdelocale.h>
#include <knotifyclient.h>
#include "calc_settings.h"
#include "calcdisplay.h"
#include "tqtlcdwidget.h"
#include "calcdisplay.moc"


#include <tqlayout.h>

CalcDisplay::CalcDisplay(TQWidget *parent, const char *name)
  :TQFrame(parent,name), _history_lcd(NULL), _main_lcd(NULL), _display_type(0), _beep(false), _groupdigits(false), _button(0), _lit(false),
   _small_text_color(0, 0, 0), _num_base(NB_DECIMAL), _word_size(0), _scientificFormat(false),
   _displayBorder(false), _displayBorderColor(0, 0, 0),
   _custom_font(font()),
   _precision(9), _fixed_precision(-1), _display_amount(0),
   selection_timer(new TQTimer)
{
	setFrameStyle(TQFrame::NoFrame);
	setBackgroundMode(TQt::PaletteBackground);
	setFocus();
	setFocusPolicy(TQWidget::StrongFocus);
	setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding, false);

	_history_label = new TQLabel(this);
	_history_label->setAlignment(AlignRight | AlignVCenter);
	_history_label->setText("");

	_history_lcd = new TQtLcdWidget(this);
	_history_lcd->setRow(1);
	_history_lcd->setColumn(32);
	_history_lcd->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);
	_history_lcd->hide();

	_main_label = new TQLabel(this);
	_main_label->setAlignment(AlignRight | AlignVCenter);
	_main_label->setText("0");

	_main_lcd = new TQtLcdWidget(this);
	_main_lcd->setRow(1);
	_main_lcd->setColumn(24);
	_main_lcd->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);
	_main_lcd->hide();
	_main_lcd->clear();
	_main_lcd->string(TQString("0").rightJustify(24));

	connect(this, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotDisplaySelected()));

	connect(selection_timer, TQ_SIGNAL(timeout()),
		this, TQ_SLOT(slotSelectionTimedOut()));

	sendEvent(EventReset);
}

CalcDisplay::~CalcDisplay()
{
	delete selection_timer;
}

bool CalcDisplay::sendEvent(Event const event)
{
	switch(event)
	{
	case EventReset:
		_display_amount = 0;
		_str_int = "0";
		_str_int_exp = TQString();

		_eestate = false;
		_period = false;
		_neg_sign = false;

		updateDisplay();

		return true;
	case EventClear:
		return sendEvent(EventReset);
	case EventChangeSign:
		return changeSign();
	case EventError:
		updateDisplay();

		return true;
	default:
		return false;
	}
}


void CalcDisplay::slotCut(void)
{
	slotCopy();
	sendEvent(EventReset);
}

void CalcDisplay::slotCopy(void)
{
	TQString txt = _main_label->text();
	if (_num_base == NB_HEX)
		txt.prepend( "0x" );
	(TQApplication::clipboard())->setText(txt, TQClipboard::Clipboard);
	(TQApplication::clipboard())->setText(txt, TQClipboard::Selection);
}

void CalcDisplay::slotPaste(bool bClipboard)
{
	TQString tmp_str = (TQApplication::clipboard())->text(bClipboard ? TQClipboard::Clipboard : TQClipboard::Selection);

	if (tmp_str.isNull())
	{
		//if (_beep)  KNotifyClient::beep();
		return;
	}

	NumBase tmp_num_base = _num_base;

	tmp_str = tmp_str.stripWhiteSpace();

	if (tmp_str.startsWith("0x", false))
	  tmp_num_base = NB_HEX;

	if (tmp_num_base != NB_DECIMAL)
	{
		bool was_ok;
		unsigned long long int tmp_result = tmp_str.toULongLong(& was_ok, tmp_num_base);

		if (!was_ok)
		{
			setAmount(KNumber::NotDefined);
			//if(_beep) KNotifyClient::beep();
			return ;
		}
	  
		setAmount(KNumber(tmp_result));
	} 
	else // _num_base == NB_DECIMAL && ! tmp_str.startsWith("0x", false)
	{
		setAmount(KNumber(tmp_str));
		//if (_beep  &&  _display_amount == KNumber::NotDefined)
			//KNotifyClient::beep();
	}
}

void CalcDisplay::slotDisplaySelected(void)
{
	if(_button == TQt::LeftButton) {
		if(_lit) {
			slotCopy();
			selection_timer->start(100);
		} else {
			selection_timer->stop();
		}

		invertColors();
	} else {
		slotPaste(false); // Selection
	}
}

void CalcDisplay::slotSelectionTimedOut(void)
{
	_lit = false;
	invertColors();
	selection_timer->stop();
}

void CalcDisplay::invertColors()
{
	// Effect removed per user request
}



void CalcDisplay::mousePressEvent(TQMouseEvent *e)
{
	if (e->button() == TQt::RightButton) {
		TQPopupMenu *menu = new TQPopupMenu(this);
		
		TQPixmap pixCopy = IconUtils::load(copy_png, copy_png_len, 32, 32);
		TQPixmap pixPaste = IconUtils::load(paste_png, paste_png_len, 32, 32);
		
		menu->insertItem(pixCopy, TQString("Copy"), this, TQ_SLOT(slotCopy()));
		menu->insertItem(pixPaste, TQString("Paste"), this, TQ_SLOT(slotPaste()));
		
		menu->exec(e->globalPos());
		delete menu;
		return;
	}

	if(e->button() == TQt::LeftButton) {
		_lit = !_lit;
		_button = TQt::LeftButton;
	} else {
		_button = TQt::MidButton;
	}
	
	emit clicked();
}

void CalcDisplay::setPrecision(int precision)
{
	_precision = precision;
}

void CalcDisplay::setFixedPrecision(int precision)
{
	if (_fixed_precision > _precision)
		_fixed_precision = -1;
	else
		_fixed_precision = precision;
}

void CalcDisplay::setBeep(bool flag)
{
	_beep = flag;
}

void CalcDisplay::setGroupDigits(bool flag)
{
	_groupdigits = flag;
}

KNumber const & CalcDisplay::getAmount(void) const
{
	return _display_amount;
}


bool CalcDisplay::setAmount(KNumber const & new_amount)
{
	TQString display_str;

	_str_int = "0";
	_str_int_exp = TQString();
	_period = false;
	_neg_sign = false;
	_eestate = false;

	if (_num_base != NB_DECIMAL  && new_amount.type() != KNumber::SpecialType)
	{
		_display_amount = new_amount.integerPart();
		unsigned long long int tmp_workaround = static_cast<unsigned long long int>(_display_amount);
		if (_word_size == 1) tmp_workaround &= 0xFFFFFFFFull;
		else if (_word_size == 2) tmp_workaround &= 0xFFFFull;
		else if (_word_size == 3) tmp_workaround &= 0xFFull;

		display_str = TQString::number(tmp_workaround, _num_base).upper();
	}
	else // _num_base == NB_DECIMAL || new_amount.type() ==
	     // KNumber::SpecialType
	{
		_display_amount = new_amount;
		if (_word_size > 0 && new_amount.type() != KNumber::SpecialType) {
			long long val = static_cast<signed long int>(new_amount);
			if (_word_size == 1) val = static_cast<int32_t>(val);
			else if (_word_size == 2) val = static_cast<int16_t>(val);
			else if (_word_size == 3) val = static_cast<int8_t>(val);
			_display_amount = KNumber(static_cast<signed long int>(val));
		}
	
		if (_scientificFormat && new_amount.type() != KNumber::SpecialType) {
			display_str = TQString::number((double)_display_amount, 'e', _fixed_precision >= 0 ? _fixed_precision : 6);
		} else {
			display_str = _display_amount.toTQString(9, _fixed_precision);
		}
#if 0
		else if (_display_amount > 1.0e+16)
			display_str = TQCString().sprintf(PRINT_LONG_BIG, _precision + 1, _display_amount);
		else
			display_str = TQCString().sprintf(PRINT_LONG_BIG, _precision, _display_amount);
#endif
	}

	setText(display_str);
	return true;
	
}

void CalcDisplay::setText(TQString const &string)
{
	TQString localizedString = string;

	// If we aren't in decimal mode, we don't need to modify the string
	/*if (_num_base == NB_DECIMAL  &&  _groupdigits)
	  // when input ends with "." (because uncomplete), the
	  // formatNumber-method does not work; fix by hand by
	  // truncating, formatting and appending again
	  if (string.endsWith(".")) {
	    localizedString.truncate(localizedString.length() - 1);
	    localizedString = TDEGlobal::locale()->formatNumber(localizedString, false, 0); // Note: rounding happened already above!
	    localizedString.append(TDEGlobal::locale()->decimalSymbol());
	  } else
	    localizedString = TDEGlobal::locale()->formatNumber(string, false, 0); // Note: rounding happened already above!
*/

	_main_label->setText(localizedString);
	if (_main_lcd) {
		_main_lcd->clear();
		TQString padded = localizedString.rightJustify(_main_lcd->currentColumn(), ' ', true);
		_main_lcd->string(padded);
	}
	emit changedText(localizedString);
}

void CalcDisplay::setWordSize(int ws)
{
	_word_size = ws;
	if (setAmount(_display_amount)) {
		updateDisplay();
	}
}

void CalcDisplay::setExpression(const TQString &expr)
{
	_history_label->setText(expr);
	if (_history_lcd) {
		_history_lcd->clear();
		TQString padded = expr.rightJustify(_history_lcd->currentColumn(), ' ', true);
		_history_lcd->string(padded);
	}
}

void CalcDisplay::clearExpression()
{
	_history_label->setText(TQString());
	if (_history_lcd) {
		_history_lcd->clear();
	}
}

void CalcDisplay::setFont(const TQFont &f)
{
	TQFrame::setFont(f);
	TQResizeEvent re(size(), size());
	resizeEvent(&re);
}

void CalcDisplay::resizeEvent(TQResizeEvent *e)
{
	TQFrame::resizeEvent(e);
	
	int W = width();
	int H = height();
	if (H < 20) return;
	
	int margin = 4;
	int spacing = 2;
	int avail_h = H - 2 * margin - spacing;
	if (avail_h < 10) return;
	
	// Divide height: 30% for history, 70% for main
	int hist_h = avail_h * 30 / 100;
	int main_h = avail_h - hist_h;
	
	// History widget geometry
	TQRect hist_rect(margin, margin, W - 2 * margin, hist_h);
	// Main widget geometry
	TQRect main_rect(margin, margin + hist_h + spacing, W - 2 * margin, main_h);
	
	if (_history_label) _history_label->setGeometry(hist_rect);
	if (_history_lcd) _history_lcd->setGeometry(hist_rect);
	if (_main_label) _main_label->setGeometry(main_rect);
	if (_main_lcd) _main_lcd->setGeometry(main_rect);
	
	TQFont f = font();
	if (_main_label) {
		TQFont main_font;
		if (_display_type == 0) main_font = TQFont("Segoe Calc");
		else if (_display_type == 2) main_font = _custom_font;
		else if (_display_type == 4) main_font = TQFont("Computo Monospace");
		else if (_display_type == 5) main_font = TQFont("Digital Counter 7");
		else if (_display_type == 6) main_font = TQFont("Pocket Calculator");
		else if (_display_type == 7) main_font = TQFont("ClassWiz Math CW");
		else main_font = f;

		if (_display_type != 2) main_font.setBold(true);
		int pxSize = main_h * 0.85;
		if (pxSize < 10) pxSize = 10;
		main_font.setPixelSize(pxSize);
		_main_label->setFont(main_font);
	}
	
	if (_history_label) {
		TQFont hist_font;
		if (_display_type == 0) hist_font = TQFont("Segoe Calc");
		else if (_display_type == 2) hist_font = _custom_font;
		else if (_display_type == 4) hist_font = TQFont("Computo Monospace");
		else if (_display_type == 5) hist_font = TQFont("Digital Counter 7");
		else if (_display_type == 6) hist_font = TQFont("Pocket Calculator");
		else if (_display_type == 7) hist_font = TQFont("ClassWiz Math CW");
		else hist_font = f;

		hist_font.setBold(false);
		int pxSize = hist_h * 0.85;
		if (pxSize < 8) pxSize = 8;
		hist_font.setPixelSize(pxSize);
		_history_label->setFont(hist_font);
	}
}

TQString CalcDisplay::text() const
{
	if (_num_base != NB_DECIMAL)
		return _main_label->text();
	TQString display_str = _display_amount.toTQString(9);

	return display_str;
}

/* change representation of display to new base (i.e. binary, decimal,
   octal, hexadecimal). The amount being displayed is changed to this
   base, but for now this amount can not be modified anymore (like
   being set with "setAmount"). Return value is the new base. */
int CalcDisplay::setBase(NumBase new_base)
{
	CALCAMNT tmp_val = static_cast<unsigned long long int>(getAmount());

	switch(new_base)
	{
	case NB_HEX:
		_num_base	= NB_HEX;
		_period 	= false;
		break;
	case NB_DECIMAL:
		_num_base	= NB_DECIMAL;
		break;
	case NB_OCTAL:
		_num_base	= NB_OCTAL;
		_period 	= false;
		break;
	case NB_BINARY:
		_num_base	= NB_BINARY;
		_period 	= false;
		break;
	default: // we shouldn't ever end up here
		_num_base	= NB_DECIMAL;
	}

	setAmount(static_cast<unsigned long long int>(tmp_val));
	
	return _num_base;
}

void CalcDisplay::setStatusText(uint i, const TQString& text)
{
	if (i < NUM_STATUS_TEXT)
		_str_status[i] = text;
	update();
}

bool CalcDisplay::updateDisplay(void)
{
	if (_str_int == "0")
	{
		return setAmount(_display_amount);
	}

	// Put sign in front.
	TQString tmp_string;
	if(_neg_sign == true)
		tmp_string = "-" + _str_int;
	else
		tmp_string = _str_int;

	switch(_num_base)
	{
	case NB_BINARY:
		Q_ASSERT(_period == false  && _eestate == false);
		setText(tmp_string);
		_display_amount = static_cast<unsigned long long int>(STRTOUL(_str_int.latin1(), 0, 2));
		if (_neg_sign)
			_display_amount = -_display_amount;
		//str_size = cvb(_str_int, boh_work, DSP_SIZE);
		break;
	  
	case NB_OCTAL:
		Q_ASSERT(_period == false  && _eestate == false);
		setText(tmp_string);
		_display_amount = static_cast<unsigned long long int>(STRTOUL(_str_int.latin1(), 0, 8));
		if (_neg_sign)
			_display_amount = -_display_amount;
		break;
		
	case NB_HEX:
		Q_ASSERT(_period == false  && _eestate == false);
		setText(tmp_string);
		_display_amount = static_cast<unsigned long long int>(STRTOUL(_str_int.latin1(), 0, 16));
		if (_neg_sign)
			_display_amount = -_display_amount;
		break;
	  
	case NB_DECIMAL:
		if(_eestate == false)
		{
			setText(tmp_string);
			_display_amount = tmp_string;
		}
		else
		{
			if(_str_int_exp.isNull())
			{
				// add 'e0' to display but not to conversion
				_display_amount = tmp_string;
				setText(tmp_string + "e0");
			}
			else
			{
				tmp_string +=  'e' + _str_int_exp;
				setText(tmp_string);
				_display_amount = tmp_string;
			}
		}
		break;
	  
	default:
	  return false;
	}

	return true;
}

void CalcDisplay::newCharacter(char const new_char)
{
	// test if character is valid
	switch(new_char)
	{
	case 'e':
		// EE can be set only once and in decimal mode
		if (_num_base != NB_DECIMAL  ||
		    _eestate == true)
		{
			//if(_beep) KNotifyClient::beep();
			return;
		}
		_eestate = true;
		break;

	case '.':
		// Period can be set only once and only in decimal
		// mode, also not in EE-mode
		if (_num_base != NB_DECIMAL  ||
		    _period == true  ||
		    _eestate == true)
		{
			//if(_beep) KNotifyClient::beep();
			return;
		}
		_period = true;
		break;

	case 'F':
	case 'E':
	case 'D':
	case 'C':
	case 'B':
	case 'A':
		if (_num_base == NB_DECIMAL)
		{
			//if(_beep) KNotifyClient::beep();
			return;
		}
		// no break
	case '9':
	case '8':
		if (_num_base == NB_OCTAL)
		{
			//if(_beep) KNotifyClient::beep();
			return;
		}
		// no break
	case '7':
	case '6':
	case '5':
	case '4':
	case '3':
	case '2':
		if (_num_base == NB_BINARY)
		{
			//if(_beep) KNotifyClient::beep();
			return;
		}
		// no break
	case '1':
	case '0':
		break;

	default:
		//if(_beep) KNotifyClient::beep();
		return;
	}

	// change exponent or mantissa
	if (_eestate)
	{
	  // ignore ',' before 'e'. turn e.g. '123.e' into '123e'
	  if (new_char == 'e'  && _str_int.endsWith( "." ))
	    {
			_str_int.truncate(_str_int.length() - 1);
			_period = false;
	    }

	  // 'e' only starts ee_mode, leaves strings unchanged
	  if (new_char != 'e'  &&
	      // do not add '0' if at start of exp
	      !(_str_int_exp.isNull() && new_char == '0'))
			_str_int_exp.append(new_char);
	}
	else
	{
		// handle first character
		if (_str_int == "0")
		{
			switch(new_char)
			{
			case '.':
				// display "0." not just "."
				_str_int.append(new_char);
				break;
			case 'e':
				// display "0e" not just "e"
				// "0e" does not make sense either, but...
				_str_int.append(new_char);
				break;
			default:
				// no leading '0's
				_str_int[0] = new_char;
			}
		}
		else
			_str_int.append(new_char);
	}

	updateDisplay();
}

void CalcDisplay::deleteLastDigit(void)
{
	// Only partially implemented !!
	if (_eestate)
	{
		if(_str_int_exp.isNull())
		{
			_eestate = false;
		}
		else
		{
			int length = _str_int_exp.length();
			if(length > 1)
			{
				_str_int_exp.truncate(length-1);
			}
			else
			{
				_str_int_exp = (char *)0;
			}
		}
	}
	else
	{
		int length = _str_int.length();
		if(length > 1)
		{
			if (_str_int[length-1] == '.')
				_period = false;
			_str_int.truncate(length-1);
		}
		else
		{
			Q_ASSERT(_period == false);
			_str_int[0] = '0';
		}
	}

	updateDisplay();
}

// change Sign of display. Problem: Only possible here, when in input
// mode. Otherwise return 'false' so that the calc_core can handle
// things.
bool CalcDisplay::changeSign(void)
{
	//stupid way, to see if in input_mode or display_mode
	if (_str_int == "0") return false;

	if(_eestate)
	{
		if(!_str_int_exp.isNull())
		{
			if (_str_int_exp[0] != '-')
				_str_int_exp.prepend('-');
			else
				_str_int_exp.remove('-');
		}
	}
	else
	{
		_neg_sign = ! _neg_sign;
	}
	
	updateDisplay();

	return true;
}

void CalcDisplay::setDisplayBorder(bool has_border, const TQColor &color)
{
	_displayBorder = has_border;
	_displayBorderColor = color;
	update();
}

void CalcDisplay::drawContents(TQPainter *p)
{
	TQFrame::drawContents(p);
	if (_displayBorder) {
		p->setPen(TQPen(_displayBorderColor, 2));
		p->drawRect(1, 1, width() - 2, height() - 2);
	}
}

void CalcDisplay::setPalette(const TQPalette &p)
{
	TQFrame::setPalette(p);
	TQColor bg = p.color(TQPalette::Active, TQColorGroup::Background);
	TQColor fg = p.color(TQPalette::Active, TQColorGroup::Foreground);
	if (_main_label) {
		_main_label->setPaletteBackgroundColor(bg);
		TQPalette main_pal = _main_label->palette();
		main_pal.setColor(TQPalette::Active, TQColorGroup::Foreground, fg);
		main_pal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, fg);
		main_pal.setColor(TQPalette::Active, TQColorGroup::Text, fg);
		main_pal.setColor(TQPalette::Inactive, TQColorGroup::Text, fg);
		_main_label->setPalette(main_pal);
	}
	if (_history_label) _history_label->setPaletteBackgroundColor(bg);

	// LCD coloring
	int r = bg.red() + (fg.red() - bg.red()) * 0.06;
	int g = bg.green() + (fg.green() - bg.green()) * 0.06;
	int b = bg.blue() + (fg.blue() - bg.blue()) * 0.06;
	TQColor offColor(r, g, b);

	if (_main_lcd) {
		_main_lcd->setColorBackground1(bg);
		_main_lcd->setColorBackground2(offColor);
		_main_lcd->setColorPixel(fg);
	}
	if (_history_lcd) {
		_history_lcd->setColorBackground1(bg);
		int hr = bg.red() + (_small_text_color.red() - bg.red()) * 0.06;
		int hg = bg.green() + (_small_text_color.green() - bg.green()) * 0.06;
		int hb = bg.blue() + (_small_text_color.blue() - bg.blue()) * 0.06;
		_history_lcd->setColorBackground2(TQColor(hr, hg, hb));
		_history_lcd->setColorPixel(_small_text_color);
	}
}

void CalcDisplay::setPaletteBackgroundColor(const TQColor &c)
{
	TQFrame::setPaletteBackgroundColor(c);
	if (_main_label) _main_label->setPaletteBackgroundColor(c);
	if (_history_label) _history_label->setPaletteBackgroundColor(c);

	if (_main_lcd) {
		_main_lcd->setColorBackground1(c);
		TQColor fg = _main_lcd->colorPixel();
		int r = c.red() + (fg.red() - c.red()) * 0.06;
		int g = c.green() + (fg.green() - c.green()) * 0.06;
		int b = c.blue() + (fg.blue() - c.blue()) * 0.06;
		_main_lcd->setColorBackground2(TQColor(r, g, b));
	}
	if (_history_lcd) {
		_history_lcd->setColorBackground1(c);
		TQColor fg = _history_lcd->colorPixel();
		int r = c.red() + (fg.red() - c.red()) * 0.06;
		int g = c.green() + (fg.green() - c.green()) * 0.06;
		int b = c.blue() + (fg.blue() - c.blue()) * 0.06;
		_history_lcd->setColorBackground2(TQColor(r, g, b));
	}
}

void CalcDisplay::setSmallTextForeground(const TQColor &color)
{
	_small_text_color = color;
	if (_history_label) {
		TQPalette hist_pal = _history_label->palette();
		hist_pal.setColor(TQPalette::Active, TQColorGroup::Foreground, _small_text_color);
		hist_pal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, _small_text_color);
		hist_pal.setColor(TQPalette::Active, TQColorGroup::Text, _small_text_color);
		hist_pal.setColor(TQPalette::Inactive, TQColorGroup::Text, _small_text_color);
		_history_label->setPalette(hist_pal);
	}

	if (_history_lcd) {
		TQColor bg = _history_lcd->colorBackground1();
		int hr = bg.red() + (color.red() - bg.red()) * 0.06;
		int hg = bg.green() + (color.green() - bg.green()) * 0.06;
		int hb = bg.blue() + (color.blue() - bg.blue()) * 0.06;
		_history_lcd->setColorBackground2(TQColor(hr, hg, hb));
		_history_lcd->setColorPixel(color);
	}
}

void CalcDisplay::setDisplayType(int type)
{
	_display_type = type;
	if (_display_type == 3) {
		// LCD mode
		if (_history_label) _history_label->hide();
		if (_main_label) _main_label->hide();
		if (_history_lcd) {
			_history_lcd->show();
			// Refresh content
			TQString expr = _history_label->text();
			_history_lcd->clear();
			TQString padded = expr.rightJustify(_history_lcd->currentColumn(), ' ', true);
			_history_lcd->string(padded);
		}
		if (_main_lcd) {
			_main_lcd->show();
			// Refresh content
			TQString mainText = _main_label->text();
			_main_lcd->clear();
			TQString padded = mainText.rightJustify(_main_lcd->currentColumn(), ' ', true);
			_main_lcd->string(padded);
		}
	} else {
		// Classic modes (0 = Default, 1 = System, 2 = Custom font, 4 = Computo Monospace, 5 = Digital Counter 7, 6 = Pocket Calculator, 7 = Casio)
		if (_history_lcd) _history_lcd->hide();
		if (_main_lcd) _main_lcd->hide();
		if (_history_label) _history_label->show();
		if (_main_label) _main_label->show();
	}

	// Force recalculation of child geometries based on current size
	TQResizeEvent re(size(), size());
	resizeEvent(&re);
}

void CalcDisplay::setCustomDisplayFont(const TQFont &font)
{
	_custom_font = font;
	if (_display_type == 2) {
		// Force recalculation of font sizes
		TQResizeEvent re(size(), size());
		resizeEvent(&re);
	}
}

TQSize CalcDisplay::sizeHint() const
{
	return TQSize(200, 72);
}

TQSize CalcDisplay::minimumSizeHint() const
{
	return TQSize(150, 48);
}

void CalcDisplay::setScientificFormat(bool scientific)
{
	_scientificFormat = scientific;
	if (_eestate) {
		// we are inputting an exponent right now, maybe don't immediately change display if it messes up input
		// actually, F-E toggles the format of the value, but if we are editing, we can update it or leave it.
		// `updateDisplay` won't format using `setAmount` while we are editing. It uses `tmp_string`
	}
	// For KCalc, usually toggling a mode doesn't retroactively format if editing, but let's just trigger updateDisplay.
	// However, if we just want to format the CURRENT _display_amount, we can call setAmount(_display_amount)
	setAmount(_display_amount);
}
