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

#include <tqglobal.h>
#include <tdeactioncollection.h>
#include <kstdaction.h>
#include <tdeconfig.h>

#include "calc_settings.h"
#include "calc_core.h"
#include "dlabel.h"
#include "dlabel.moc"



DispLogic::DispLogic(TQWidget *parent, const char *name)
  :CalcDisplay(parent,name), _history_index(0)
{
	KNumber::setDefaultFloatOutput(true);
	KNumber::setDefaultFractionalInput(true);
	//_back = KStdAction::undo(this, TQ_SLOT(history_back()), coll);
	//_forward = KStdAction::redo(this, TQ_SLOT(history_forward()), coll);

	//_forward->setEnabled(false);
	//_back->setEnabled(false);
}

DispLogic::~DispLogic()
{
}

void DispLogic::changeSettings()
{
	TDEConfig *config = new TDEConfig("calcrc");
	config->setGroup("Preferences");
	int mode = config->readNumEntry("AppearanceMode", 2);
	TQString selectedTheme = config->readEntry("SelectedTheme", "Midnight Blue");

	TQColor defaultDisplayFg(0, 0, 0);
	TQColor defaultDisplayBg(189, 255, 180);

	TQColor colorDisplayFg = defaultDisplayFg;
	TQColor colorDisplayBg = defaultDisplayBg;

	if (mode == 0) { // Classic Dark
		colorDisplayFg = TQColor(255, 255, 255);
		colorDisplayBg = TQColor(0, 0, 0);
	} else if (mode == 2) { // Classic
		colorDisplayFg = TQColor(0, 0, 0);
		colorDisplayBg = TQColor(242, 242, 242);
	} else if (mode == 3) { // Theme
		if (selectedTheme == "Midnight Blue") {
			colorDisplayFg = TQColor(0, 255, 204);
			colorDisplayBg = TQColor(13, 21, 32);
		} else if (selectedTheme == "Forest Green") {
			colorDisplayFg = TQColor(57, 255, 20);
			colorDisplayBg = TQColor(11, 19, 14);
		} else if (selectedTheme == "Classic Gray") {
			colorDisplayFg = TQColor(242, 242, 242);
			colorDisplayBg = TQColor(126, 126, 126);
		} else if (selectedTheme == "Coloured") {
			colorDisplayFg = TQColor(255, 255, 255);
			colorDisplayBg = TQColor(11, 19, 14);
		} else if (selectedTheme == "orange style") {
			colorDisplayFg = TQColor(253, 139, 63);
			colorDisplayBg = TQColor(11, 19, 14);
		} else if (selectedTheme == "Default" || selectedTheme == "internal_classic") {
			colorDisplayFg = TQColor(0, 0, 0);
			colorDisplayBg = TQColor(242, 242, 242);
		} else if (selectedTheme == "internal_classic_dark") {
			colorDisplayFg = TQColor(255, 255, 255);
			colorDisplayBg = TQColor(0, 0, 0);
		} else {
			TDEConfig *themeConfig = new TDEConfig("calcrc");
			themeConfig->setGroup("Theme_" + selectedTheme);
			colorDisplayFg = themeConfig->readColorEntry("DisplayFgColor", &defaultDisplayFg);
			colorDisplayBg = themeConfig->readColorEntry("DisplayBgColor", &defaultDisplayBg);
			delete themeConfig;
		}
	} else { // Custom
		colorDisplayFg = config->readColorEntry("CustomDisplayFgColor", &defaultDisplayFg);
		colorDisplayBg = config->readColorEntry("CustomDisplayBgColor", &defaultDisplayBg);
	}
	delete config;

	TQPalette pal = palette();
	pal.setColor(TQPalette::Active, TQColorGroup::Text, colorDisplayFg);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Text, colorDisplayFg);
	pal.setColor(TQPalette::Active, TQColorGroup::Foreground, colorDisplayFg);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, colorDisplayFg);
	pal.setColor(TQPalette::Active, TQColorGroup::Background, colorDisplayBg);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Background, colorDisplayBg);

	setPalette(pal);
	setPaletteBackgroundColor(colorDisplayBg);

	setFont(TQFont());

	setPrecision(9);

	if(false == false)
		setFixedPrecision(-1);
	else
		setFixedPrecision(4);

	setBeep(false);
	setGroupDigits(false);
	updateDisplay();
}

void DispLogic::update_from_core(CalcEngine const &core,
				 bool store_result_in_history)
{
	bool tmp_error;
	KNumber const & output = core.lastOutput(tmp_error);
	if(tmp_error) sendEvent(EventError);
	if (setAmount(output)  &&  store_result_in_history  &&
	    output != KNumber::Zero)
	{
	  // add this latest value to our history
	  _history_list.insert(_history_list.begin(), output);
	  _history_index = 0;
	  //_back->setEnabled(true);
	  //_forward->setEnabled(false);
	}
}

void DispLogic::EnterDigit(int data)
{
	char tmp;
	switch(data)
	{
	case 0:
	  tmp = '0';
	  break;
	case 1:
	  tmp = '1';
	  break;
	case 2:
	  tmp = '2';
	  break;
	case 3:
	  tmp = '3';
	  break;
	case 4:
	  tmp = '4';
	  break;
	case 5:
	  tmp = '5';
	  break;
	case 6:
	  tmp = '6';
	  break;
	case 7:
	  tmp = '7';
	  break;
	case 8:
	  tmp = '8';
	  break;
	case 9:
	  tmp = '9';
	  break;
	case 0xA:
	  tmp = 'A';
	  break;
	case 0xB:
	  tmp = 'B';
	  break;
	case 0xC:
	  tmp = 'C';
	  break;
	case 0xD:
	  tmp = 'D';
	  break;
	case 0xE:
	  tmp = 'E';
	  break;
	case 0xF:
	  tmp = 'F';
	  break;
	default:
	  tmp = '?';
	  break;
	}

	newCharacter(tmp);
}

void DispLogic::history_forward()
{
	Q_ASSERT(! _history_list.empty());
	Q_ASSERT(_history_index > 0);

	_history_index --;

	setAmount(_history_list[_history_index]);

	//if(_history_index == 0) _forward->setEnabled(false);

	//_back->setEnabled(true);
}

void DispLogic::history_back()
{
	Q_ASSERT(! _history_list.empty());
	Q_ASSERT( _history_index < static_cast<int>(_history_list.size()) );

	setAmount(_history_list[_history_index]);

	_history_index ++;
	
	//if( _history_index == static_cast<int>(_history_list.size()) )
		//_back->setEnabled(false);
	//_forward->setEnabled(true);
}

