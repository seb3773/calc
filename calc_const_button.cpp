/*
    kCalculator, a simple scientific calculator for KDE

    Copyright (C) 2003 Klaus Niederkrueger <kniederk@math.uni-koeln.de>

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

#include <tqstring.h>


#include <kcmenumngr.h>
#include <kinputdialog.h>
#include <tqpopupmenu.h>

#include "calc_const_button.h"
#include "calc_const_menu.h"
#include "calc_settings.h"


CalcConstButton::CalcConstButton(TQWidget *parent, int but_num, const char * name)
  : CalcButton(parent, name), _button_num(but_num)
{
  addMode(ModeInverse, "Store", TQString("Write display data into memory"));
  
  initPopupMenu();
}


CalcConstButton::CalcConstButton(const TQString &label, TQWidget *parent, int but_num,
                                   const char * name, const TQString &tooltip)
  : CalcButton(label, parent, name, tooltip), _button_num(but_num)
{
  addMode(ModeInverse, "Store", TQString("Write display data into memory"));
  
  initPopupMenu();
}

TQString CalcConstButton::constant(void) const
{
  return TQString("0");
}

void CalcConstButton::setLabelAndTooltip(void)
{
  TQString new_label = TQString("C") + TQString().setNum(_button_num + 1);
  TQString new_tooltip;
  
  new_label = TQString("C");
  
  new_tooltip = new_label + "=" + TQString("0");
  
  addMode(ModeNormal, new_label, new_tooltip);
}

void CalcConstButton::initPopupMenu(void)
{
  CalcConstMenu *tmp_menu = new CalcConstMenu(this);
  
  _popup = new TQPopupMenu(this, "set const-cutton");
  _popup->insertItem(TQString("Set Name"), 0);
  _popup->insertItem(TQString("Choose From List"), tmp_menu, 1);
  
  connect(_popup, TQ_SIGNAL(activated(int)), TQ_SLOT(slotConfigureButton(int)));
  connect(tmp_menu, TQ_SIGNAL(activated(int)), TQ_SLOT(slotChooseScientificConst(int)));

  //KContextMenuManager::insert(this, _popup);
}

void CalcConstButton::slotConfigureButton(int option)
{
  if (option == 0)
    {
      /*bool yes_no;
      TQString input = KInputDialog::text(TQString("New Name for Constant"), TQString("New name:"),
					 text(), &yes_no, this, "nameUserConstants-Dialog");
      if(yes_no) {
	//CalcSettings::setNameConstant(_button_num, input);
	setLabelAndTooltip();
      }*/
    }
}

void CalcConstButton::slotChooseScientificConst(int option)
{
  //CalcSettings::setValueConstant(_button_num,
  //				  CalcConstMenu::Constants[option].value);

  //CalcSettings::setNameConstant(_button_num,
  //				 CalcConstMenu::Constants[option].label);
  
  setLabelAndTooltip();
}

#include "calc_const_button.moc"

