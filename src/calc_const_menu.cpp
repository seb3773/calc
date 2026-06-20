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

#include <tdelocale.h>

#include "calc_const_menu.h"

// Removed hardcoded NUM_CONST
const struct science_constant CalcConstMenu::Constants[] = {
  {TQString::fromUtf8("π"), I18N_NOOP("Pi"), "",
   "3.1415926535897932384626433832795028841971693993751"
   "05820974944592307816406286208998628034825342117068", Mathematics},
  {TQString::fromUtf8("τ"), I18N_NOOP("Tau"), "",
   "6.2831853071795864769252867665590057683943387987502"
   "11641949889184615632812572417997256069650684234136", Mathematics},
  {"e", I18N_NOOP("Euler Number"), "",
   "2.7182818284590452353602874713526624977572470936999"
   "59574966967627724076630353547594571382178525166427", Mathematics},
  {TQString::fromUtf8("φ"), I18N_NOOP("Golden Ratio"), "", "1.61803398874989484820458683436563811", Mathematics},
  {"c", I18N_NOOP("Light Speed"), "", "2.99792458e8", Electromagnetic},
  {"h", I18N_NOOP("Planck's Constant"), "", "6.6260693e-34", Nuclear},
  {"G", I18N_NOOP("Constant of Gravitation"), "", "6.6742e-11", Gravitation},
  {"g", I18N_NOOP("Earth Acceleration"), "", "9.80665", Gravitation},
  {"e", I18N_NOOP("Elementary Charge"), "", "1.60217653e-19", ConstantCategory(Electromagnetic|Nuclear)},
  {"Z_0", I18N_NOOP("Impedance of Vacuum"), "", "376.730313461", Electromagnetic},
  {TQString::fromUtf8("α"), I18N_NOOP("Fine-Structure Constant"), "", "7.297352568e-3", Nuclear},
  {TQString::fromUtf8("μ")+"_0", I18N_NOOP("Permeability of Vacuum"), "", "1.2566370614e-6", Electromagnetic},
  {TQString::fromUtf8("ε")+"_0", I18N_NOOP("Permittivity of vacuum"), "", "8.854187817e-12", Electromagnetic},
  {"k", I18N_NOOP("Boltzmann Constant"), "", "1.3806505e-23", Thermodynamics},
  {"1u", I18N_NOOP("Atomic Mass Unit"), "", "1.66053886e-27", Thermodynamics},
  {"R", I18N_NOOP("Molar Gas Constant"), "", "8.314472", Thermodynamics},
  {TQString::fromUtf8("σ"), I18N_NOOP("Stefan-Boltzmann Constant"), "", "5.670400e-8", Thermodynamics},
  {"N_A", I18N_NOOP("Avogadro's Number"), "", "6.0221415e23", Thermodynamics},
  {"F", I18N_NOOP("Faraday Constant"), "", "96485.3321", Chemistry},
  {"e", I18N_NOOP("Elementary Charge"), "", "1.602176634e-19", Chemistry},
  {"1 B", I18N_NOOP("Bits in a Byte"), "", "8", Computing},
  {"1 KiB", I18N_NOOP("Bytes in a Kilobyte (KiB)"), "", "1024", Computing},
  {"1 MiB", I18N_NOOP("Bytes in a Megabyte (MiB)"), "", "1048576", Computing},
  {"1 GiB", I18N_NOOP("Bytes in a Gigabyte (GiB)"), "", "1073741824", Computing},
  {"1 TiB", I18N_NOOP("Bytes in a Terabyte (TiB)"), "", "1099511627776", Computing},
  {"Day", I18N_NOOP("Seconds in a Day"), "", "86400", Computing},
  {"1 AU", I18N_NOOP("Astronomical Unit (m)"), "", "1.495978707e11", Astrophysics},
  {"1 ly", I18N_NOOP("Light Year (m)"), "", "9.4607304725808e15", Astrophysics},
  {"1 pc", I18N_NOOP("Parsec (m)"), "", "3.08567758e16", Astrophysics},
  {"M_sun", I18N_NOOP("Solar Mass (kg)"), "", "1.98847e30", Astrophysics},
  {"1 tbsp", I18N_NOOP("Tablespoon (ml)"), "", "14.7868", Culinary},
  {"1 tsp", I18N_NOOP("Teaspoon (ml)"), "", "4.92892", Culinary},
  {"1 cup", I18N_NOOP("Cup (ml)"), "", "236.588", Culinary},
  {"1 oz", I18N_NOOP("Ounce (g)"), "", "28.3495", Culinary},
  {"1 fl oz", I18N_NOOP("Fluid Ounce (ml)"), "", "29.5735", Culinary}
};

CalcConstMenu::CalcConstMenu(TQWidget * parent, const char * name)
  : TQPopupMenu(parent, name)
{
  TQPopupMenu *math_menu = new TQPopupMenu(this, "mathematical constants");
  TQPopupMenu *em_menu = new TQPopupMenu(this, "electromagnetic constants");
  TQPopupMenu *nuclear_menu = new TQPopupMenu(this, "nuclear constants");
  TQPopupMenu *thermo_menu = new TQPopupMenu(this, "thermodynamics constants");
  TQPopupMenu *gravitation_menu = new TQPopupMenu(this, "gravitation constants");
  TQPopupMenu *chemistry_menu = new TQPopupMenu(this, "chemistry constants");
  TQPopupMenu *computing_menu = new TQPopupMenu(this, "computing constants");
  TQPopupMenu *astro_menu = new TQPopupMenu(this, "astrophysics constants");
  TQPopupMenu *culinary_menu = new TQPopupMenu(this, "culinary constants");

  insertItem(TQString("Mathematics"), math_menu);
  insertItem(TQString("Electromagnetism"), em_menu);
  insertItem(TQString("Atomic && Nuclear"), nuclear_menu);
  insertItem(TQString("Thermodynamics"), thermo_menu);
  insertItem(TQString("Gravitation"), gravitation_menu);
  insertItem(TQString("Chemistry"), chemistry_menu);
  insertItem(TQString("Computing"), computing_menu);
  insertItem(TQString("Astrophysics"), astro_menu);
  insertItem(TQString("Everyday && Cuisine"), culinary_menu);

  connect(math_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(em_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(nuclear_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(thermo_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(gravitation_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(chemistry_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(computing_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(astro_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));
  connect(culinary_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotPassActivate(int)));


  int num_constants = sizeof(Constants)/sizeof(Constants[0]);
  for (int i = 0; i<num_constants; i++) {
    if(Constants[i].category  &  Mathematics)
      math_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Electromagnetic)
      em_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Nuclear)
      nuclear_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Thermodynamics)
      thermo_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Gravitation)
      gravitation_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Chemistry)
      chemistry_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Computing)
      computing_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Astrophysics)
      astro_menu->insertItem(TQString(Constants[i].name), i);
    if(Constants[i].category  &  Culinary)
      culinary_menu->insertItem(TQString(Constants[i].name), i);
  }
}


#include "calc_const_menu.moc"
