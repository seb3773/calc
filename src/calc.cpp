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

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>


#include <tqbuttongroup.h>
#include <tqfont.h>
#include <tqhbuttongroup.h>
#include <tqlabel.h>
#include <tqlayout.h>
#include <tqobjectlist.h>
#include <tqradiobutton.h>
#include <tqspinbox.h>
#include <tqstyle.h>
#include <tqtooltip.h>
#include <tqapplication.h>



#include <tdeaboutdata.h>
#include <tdeaccel.h>
#include <tdeaction.h>
#include <tdeapplication.h>
#include <tdecmdlineargs.h>
#include <kcolorbutton.h>
#include <kcolordrag.h>
#include <tdeconfig.h>
#include <tdeconfigdialog.h>
#include <kdialog.h>
#include <tdefontdialog.h>
#include <tdeglobal.h>
#include <tdeglobalsettings.h>
#include <kkeydialog.h>
#include <tdemenubar.h>
#include <knotifyclient.h>
#include <knumvalidator.h>
#include <tqpopupmenu.h>
#include <tqpixmap.h>
#include <tqclipboard.h>
#include <tqimage.h>
#include <tqmime.h>
#include <tqsimplerichtext.h>
#include <tqtimer.h>
#include "embedded_icons.h"
#include <kpushbutton.h>
#include <kstatusbar.h>
#include <kstdaction.h>

#include "dlabel.h"
#include "tqtlcdwidget.h"
#include "calc.h"
#include "history_window.h"
#include "translation.h"
#include "calc_const_menu.h"
#include "version.h"

#include <tdeaccelmanager.h>
#include "calc_settings.h"
#include <twin.h>
#include "settings_dialog.h"
#include <tqpainter.h>
#include <tqbitmap.h>
#include <tqcombobox.h>
#include "tqtdateperiodpicker.h"
#include "icon_utils.h"
#include <tqmenudata.h>

class CalcMenuItem : public TQCustomMenuItem {
public:
    CalcMenuItem(const unsigned char* mode_icon_data, unsigned int mode_icon_len, const TQString& text, bool isChecked, bool isMenuBarMenu = false) 
        : m_text(text), m_isChecked(isChecked), m_isMenuBarMenu(isMenuBarMenu) {
        
        if (mode_icon_data && mode_icon_len > 0) {
            m_modeIcon = IconUtils::load(mode_icon_data, mode_icon_len, 24, 24);
        }
        
        if (isChecked) {
            TQImage img = IconUtils::loadImageRaw(list_check_png, list_check_png_len);
            int checkW = (img.width() * 24) / img.height();
            m_checkIcon = IconUtils::load(list_check_png, list_check_png_len, checkW, 24);
        }
    }

    void setChecked(bool checked) {
        if (m_isChecked != checked) {
            m_isChecked = checked;
            if (m_isChecked && m_checkIcon.isNull()) {
                TQImage img = IconUtils::loadImageRaw(list_check_png, list_check_png_len);
                int checkW = (img.width() * 24) / img.height();
                m_checkIcon = IconUtils::load(list_check_png, list_check_png_len, checkW, 24);
            }
        }
    }
    
    bool fullSpan() const { return true; }
    
    void paint(TQPainter* p, const TQColorGroup& cg, bool act, bool enabled, int x, int y, int w, int h) {
        if (act) {
            p->fillRect(x, y, w, h, cg.highlight());
            p->setPen(cg.highlightedText());
        } else {
            p->fillRect(x, y, w, h, cg.background());
            p->setPen(cg.buttonText());
        }
        
        int currentX = x + 6;
        int centerY = y + h/2;
        
        if (m_isChecked && !m_checkIcon.isNull()) {
            p->drawPixmap(currentX, centerY - m_checkIcon.height()/2, m_checkIcon);
        }
        currentX += 5; // Tighter space
        
        if (!m_modeIcon.isNull()) {
            p->drawPixmap(currentX, centerY - m_modeIcon.height()/2, m_modeIcon);
        }
        currentX += 30; // 24px icon + 6px padding
        
        p->drawText(currentX, y, w - currentX - 8, h, TQt::AlignVCenter | TQt::AlignLeft, m_text);
    }
    
    TQSize sizeHint() {
        return TQSize(190, 32);
    }
    
private:
    TQPixmap m_checkIcon;
    TQPixmap m_modeIcon;
    TQString m_text;
    bool m_isChecked;
    bool m_isMenuBarMenu;
};

#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* Structure MWM (Motif Window Manager Hints) */
typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long          input_mode;
    unsigned long status;
} MWMHints;

#define MWM_HINTS_DECORATIONS  (1UL << 1)
#define MWM_DECOR_NONE         0UL
#define MWM_DECOR_ALL          1UL



static const char description[] = I18N_NOOP("TDE Calculator");
static const char version[] = CALCVERSION;

static void applyIconToButton(CalcButton* btn, const unsigned char* data, unsigned int len, const TQString& tooltip) {
    if (!btn) return;
    btn->setCustomIcon(data, len);
    btn->addMode(ModeNormal, "", tooltip);
}


CalcCentralWidget::CalcCentralWidget(Calculator *calc, TQWidget *parent)
	: TQWidget(parent), m_calc(calc)
{
}

void CalcCentralWidget::paintEvent(TQPaintEvent *e)
{
	TQWidget::paintEvent(e);
	if (m_calc->hasWindowBorder()) {
		TQPainter painter(this);
		painter.setPen(TQPen(m_calc->windowBorderColor(), 2));
		painter.drawRect(1, 1, width() - 2, height() - 2);
	}
}


Calculator::Calculator(TQWidget *parent, const char *name)
	: TQMainWindow(parent, name), inverse(false),
	  hyp_mode(false), memory_num(0.0), calc_display(NULL),
	  mInternalSpacing(4), is_pinned(false), m_noDeco(false),
	  m_windowBorder(false), m_windowBorderColor(0, 0, 0), _word_size(0),
	  _expr_just_equaled(false), _expr_op_pending(false), _calc_mode(ModeStandard), core(),
	  _angle_mode(DegMode),
	  btnProgM(NULL), mConverterPage(NULL),
	  m_converterTopPanel(NULL),
	  mProgrammerMemoryFrame(NULL),
	  mProgMemoryFrameScroll(NULL),
	  mProgMemoryContainer(NULL),
	  mProgMemoryBottomBar(NULL),
	  m_programmerMemoryFrameVisible(false), m_programmer_bitboard_view(false),
	  mDateCalcPage(NULL), mDateModeCombo(NULL),
	  mDateFromPicker(NULL), mDateToPicker(NULL),
	  mDateDiffLabel(NULL), mDateDiffResult(NULL),
	  mDateDiffResultDays(NULL), lblFrom(NULL),
	  m_comboUnit1(NULL), m_comboUnit2(NULL),
	  m_lblVal1(NULL), m_lblVal2(NULL),
	  m_lblEquiv(NULL), m_lblEquivTitle(NULL),
	  mBitboardPage(NULL), mScientificPage(NULL),
	  mScientificPageGrid(NULL), mStandardPage(NULL),
	  mStandardPageGrid(NULL), mProgrammerPage(NULL),
	  mProgrammerPageGrid(NULL),
	  lblTo(NULL), mAddRadio(NULL), mSubRadio(NULL),
	  lblYears(NULL), lblMonths(NULL), lblDays(NULL),
	  mYearsCombo(NULL), mMonthsCombo(NULL), mDaysCombo(NULL),
	  mRadioLayout(NULL), mOffsetLayout(NULL),
	  colYears(NULL), colMonths(NULL), colDays(NULL),
	  pbSciInv(NULL), pbSciSin(NULL), pbSciCos(NULL), pbSciTan(NULL),
	  pbSciParenL(NULL), pbSciParenR(NULL),
	  pbSciEq(NULL), pbSciC(NULL), pbSciCE(NULL), pbSciBS(NULL),
	  pbSciDiv(NULL), pbSciMul(NULL), pbSciSub(NULL), pbSciAdd(NULL),
	  pbSciDot(NULL), pbSciPM(NULL),
	  pbProgEq(NULL), pbProgC(NULL), pbProgCE(NULL), pbProgBS(NULL),
	  pbProgDiv(NULL), pbProgMul(NULL), pbProgSub(NULL), pbProgAdd(NULL),
	  pbProgPM(NULL), pbProgParenL(NULL), pbProgParenR(NULL),
	  m_isDragging(false), m_lblVal1_lcd(NULL), m_lblVal2_lcd(NULL),
	  m_displayType(0), m_displayFont("Courier New", 12),
	  m_colorDisplayBg(189, 255, 180), m_colorDisplayFg(0, 0, 0),
	  m_colorPanelBg(230, 230, 230), m_colorPanelFg(0, 0, 0),
	  m_colorMenuBg(230, 230, 230),
	  m_historyWindow(NULL)
{
	for (int i = 0; i < 16; ++i) mBitboardLabels[i] = NULL;

	// Initialize converter mode state arrays
	for (int i = 0; i < 13; ++i) {
		m_convSelectedUnit1[i] = 0;
		m_convSelectedUnit2[i] = 1;
		m_convSelectedInput1[i] = "0";
		m_convSelectedInput2[i] = "0";
		m_convSelectedActiveSlot[i] = 1;
	}
	m_convSelectedUnit1[0] = 8; // Volume: Quarts
	m_convSelectedUnit2[0] = 0; // Volume: Milliliters
	m_convSelectedUnit1[1] = 2; // Length: Meters
	m_convSelectedUnit2[1] = 4; // Length: Inches
	m_convSelectedUnit2[2] = 4; // Mass: Pounds (default unit1 is 0, Grams. Let's make unit1=1, Kilograms)
	m_convSelectedUnit1[2] = 1; // Mass: Kilograms
	m_convSelectedUnit1[3] = 0; // Temp: Celsius
	m_convSelectedUnit2[3] = 1; // Temp: Fahrenheit
	m_convSelectedUnit1[4] = 3; // Energy: Kilocalories
	m_convSelectedUnit2[4] = 0; // Energy: Joules
	m_convSelectedUnit1[5] = 2; // Area: Square meters
	m_convSelectedUnit2[5] = 7; // Area: Acres
	m_convSelectedUnit1[6] = 1; // Speed: km/h
	m_convSelectedUnit2[6] = 2; // Speed: mph
	m_convSelectedUnit1[7] = 3; // Time: Hours
	m_convSelectedUnit2[7] = 2; // Time: Minutes
	m_convSelectedUnit1[8] = 1; // Power: Kilowatts
	m_convSelectedUnit2[8] = 3; // Power: Horsepower
	m_convSelectedUnit1[9] = 3; // Data: Megabytes
	m_convSelectedUnit2[9] = 4; // Data: Gigabytes
	m_convSelectedUnit1[10] = 2; // Pressure: Bars
	m_convSelectedUnit2[10] = 4; // Pressure: PSI
	m_convSelectedUnit1[11] = 0; // Angle: Degrees
	m_convSelectedUnit2[11] = 1; // Angle: Radians
	m_convSelectedUnit1[12] = 0; // Roman: Arabic
	m_convSelectedUnit2[12] = 0; // Roman: Roman (index 0 because there is only one Roman item in dropdown)
	m_currentLoadedModeIdx = -1;

	/* central widget to contain all the elements */
	CalcCentralWidget *central = new CalcCentralWidget(this, this);
	setCentralWidget(central);
	setupMenuBar();
	TDEAcceleratorManager::setNoAccel( this );

	// Detect color change
	//connect(tdeApp,TQ_SIGNAL(tdedisplayPaletteChanged()), TQ_SLOT(set_colors()));

	TQImage calcImg = IconUtils::loadImageRaw(calc_png, calc_png_len);
	setIcon(TQPixmap(calcImg));

	calc_display = new DispLogic(central, "display"/*, actionCollection()*/);
	calc_display->installEventFilter(this);

	//setupMainActions();

	//setupStatusbar();

	//createGUI();

	// How can I make the toolBar not appear at all?
	// This is not a nice solution.
	//toolBar()->close();

	// Create Button to select BaseMode
	BaseChooseGroup = new TQHButtonGroup(TQString("Base"), central);
	connect(BaseChooseGroup, TQ_SIGNAL(clicked(int)), TQ_SLOT(slotBaseSelected(int)));
	BaseChooseGroup->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Fixed, false);


	pbBaseChoose[0] =  new TQRadioButton(TQString("He&x"), BaseChooseGroup,
					    "Hexadecimal-Switch");
	//TQToolTip::add(pbBaseChoose[0], TQString("Switch base to hexadecimal."));

	pbBaseChoose[1] =  new TQRadioButton(TQString("&Dec"), BaseChooseGroup,
					    "Decimal-Switch");
	//TQToolTip::add(pbBaseChoose[1], TQString("Switch base to decimal."));

	pbBaseChoose[2] =  new TQRadioButton(TQString("&Oct"), BaseChooseGroup,
					    "Octal-Switch");
	//TQToolTip::add(pbBaseChoose[2], TQString("Switch base to octal."));

	pbBaseChoose[3] =  new TQRadioButton(TQString("&Bin"), BaseChooseGroup,
					    "Binary-Switch");
	//TQToolTip::add(pbBaseChoose[3], TQString("Switch base to binary."));


	// Create Button to select AngleMode
	pbAngleChoose =  new CalcButton(TQString("&Angle"),
					 central, "ChooseAngleMode-Button");
	pbAngleChoose->setButtonType(CalcButton::TypeHeader);
	//TQToolTip::add(pbAngleChoose, TQString("Choose the unit for the angle measure"));
	pbAngleChoose->setAutoDefault(false);

	TQPopupMenu *angle_menu = new TQPopupMenu(pbAngleChoose, "AngleMode-Selection-Menu");
	angle_menu->insertItem(TQString("Degrees"), 0);
	angle_menu->insertItem(TQString("Radians"), 1);
	angle_menu->insertItem(TQString("Gradians"), 2);

	angle_menu->setCheckable(true);
	connect(angle_menu, TQ_SIGNAL(activated(int)), TQ_SLOT(slotAngleSelected(int)));
	pbAngleChoose->setPopup(angle_menu);



	pbInv = new CalcButton("Inv", central, "Inverse-Button",
				TQString("Inverse mode"));
	pbInv->setAccel(Key_I);
	connect(pbInv, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotInvtoggled(bool)));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbInv, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	pbInv->setToggleButton(true);

	//
	//  Create Calculator Buttons
	//

	// First the widgets that are the parents of the buttons
	mSmallPage = new TQWidget(central);
	mLargePage = new TQWidget(central);
	mNumericPage = 	setupNumericKeys(central);

	setupLogicKeys(mSmallPage);
	setupStatisticKeys(mSmallPage);
	setupScientificKeys(mSmallPage);
	setupConstantsKeys(mSmallPage);

	// Windows 10/11 style header and memory row
	setupHeaderBar(central);
	setupStandardKeys(central);
	setupScientificKeys_win10(central);
	setupProgrammerKeys_win10(central);

	pbMod = new CalcButton(mSmallPage, "Modulo-Button");
	pbMod->addMode(ModeNormal, "Mod", TQString("Modulo"));
	pbMod->addMode(ModeInverse, "IntDiv", TQString("Integer division"));
	pbMod->setAccel(Key_Colon);
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		pbMod, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbMod, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbMod, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotModclicked(void)));

	pbReci = new CalcButton(mSmallPage, "Reciprocal-Button");
	pbReci->addMode(ModeNormal, "1/x", TQString("Reciprocal"));
	pbReci->setAccel(Key_R);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbReci, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbReci, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotReciclicked(void)));

	pbFactorial = new CalcButton(mSmallPage, "Factorial-Button");
	pbFactorial->addMode(ModeNormal, "x!", TQString("Factorial"));
	pbFactorial->setAccel(Key_Exclam);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbFactorial, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbFactorial, TQ_SIGNAL(clicked(void)),TQ_SLOT(slotFactorialclicked(void)));

	// Representation of x^2 is moved to the function
	// changeRepresentation() that paints the letters When
	// pressing the INV Button a sqrt symbol will be drawn on that
	// button
	pbSquare = new CalcButton(mSmallPage, "Square-Button");
	pbSquare->addMode(ModeNormal, "x<sup>2</sup>", TQString("Square"), true);
	pbSquare->addMode(ModeInverse, "x<sup>3</sup>", TQString("Third power"), true);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSquare, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
                pbSquare, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(pbSquare, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotSquareclicked(void)));

	pbRoot = new KSquareButton(mSmallPage, "Square-Button");
	pbRoot->addMode(ModeNormal, "sqrt(x)", TQString("Square root"));
	pbRoot->addMode(ModeInverse, "sqrt[3](x)", TQString("Cube root"));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbRoot, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
                pbRoot, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(pbRoot, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotRootclicked(void)));


	// Representation of x^y is moved to the function
	// changeRepresentation() that paints the letters When
	// pressing the INV Button y^x will be drawn on that button
	pbPower = new CalcButton(mSmallPage, "Power-Button");
	pbPower->addMode(ModeNormal, "x<sup>y</sup>", TQString("x to the power of y"), true);
	pbPower->addMode(ModeInverse, "x<sup>1/y</sup>", TQString("x to the power of 1/y"), true);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbPower, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		pbPower, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	pbPower->setAccel(Key_AsciiCircum);
	connect(pbPower, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPowerclicked(void)));


	//
	// All these layouts are needed because all the groups have their
	// own size per row so we can't use one huge TQGridLayout (mosfet)
	//
	TQGridLayout *smallBtnLayout = new TQGridLayout(mSmallPage, 6, 4, 0,
		mInternalSpacing);
	TQGridLayout *largeBtnLayout = new TQGridLayout(mLargePage, 5, 2, 0,
		mInternalSpacing);

	TQHBoxLayout *topLayout		= new TQHBoxLayout();
	TQHBoxLayout *btnLayout		= new TQHBoxLayout();

	// bring them all together
	TQVBoxLayout *mainLayout = new TQVBoxLayout(central, mInternalSpacing, 2);

	// Add grids below memory row
	mainLayout->addWidget(mStandardPageGrid, 1);
	mainLayout->addWidget(mScientificPageGrid, 1);
	mainLayout->addWidget(mProgrammerPageGrid, 1);
	mainLayout->addLayout(topLayout);
	mainLayout->addLayout(btnLayout);

	// button layout
	btnLayout->addWidget(mSmallPage, 0, AlignTop);
	btnLayout->addSpacing(2*mInternalSpacing);
	btnLayout->addWidget(mNumericPage, 0, AlignTop);
	btnLayout->addSpacing(2*mInternalSpacing);
	btnLayout->addWidget(mLargePage, 0, AlignTop);

	// small button layout
	smallBtnLayout->addWidget(pbStat["NumData"], 0, 0);
	smallBtnLayout->addWidget(pbScientific["HypMode"], 0, 1);
	smallBtnLayout->addWidget(pbLogic["AND"], 0, 2);
	smallBtnLayout->addWidget(pbMod, 0, 3);
	smallBtnLayout->addWidget(NumButtonGroup->find(0xA), 0, 4);
	smallBtnLayout->addWidget(pbConstant[0], 0, 5);

	smallBtnLayout->addWidget(pbStat["Mean"], 1, 0);
	smallBtnLayout->addWidget(pbScientific["Sine"], 1, 1);
	smallBtnLayout->addWidget(pbLogic["OR"], 1, 2);
	smallBtnLayout->addWidget(pbReci, 1, 3);
	smallBtnLayout->addWidget(NumButtonGroup->find(0xB), 1, 4);
	smallBtnLayout->addWidget(pbConstant[1], 1, 5);

	smallBtnLayout->addWidget(pbStat["StandardDeviation"], 2, 0);
	smallBtnLayout->addWidget(pbScientific["Cosine"], 2, 1);
	smallBtnLayout->addWidget(pbLogic["XOR"], 2, 2);
	smallBtnLayout->addWidget(pbFactorial, 2, 3);
	smallBtnLayout->addWidget(NumButtonGroup->find(0xC), 2, 4);
	smallBtnLayout->addWidget(pbConstant[2], 2, 5);

	smallBtnLayout->addWidget(pbStat["Median"], 3, 0);
	smallBtnLayout->addWidget(pbScientific["Tangent"], 3, 1);
	smallBtnLayout->addWidget(pbLogic["LeftShift"], 3, 2);
	smallBtnLayout->addWidget(pbSquare, 3, 3);
	smallBtnLayout->addWidget(NumButtonGroup->find(0xD), 3, 4);
	smallBtnLayout->addWidget(pbConstant[3], 3, 5);

	smallBtnLayout->addWidget(pbStat["InputData"], 4, 0);
	smallBtnLayout->addWidget(pbScientific["Log10"], 4, 1);
	smallBtnLayout->addWidget(pbLogic["RightShift"], 4, 2);
	smallBtnLayout->addWidget(pbRoot, 4, 3);
	smallBtnLayout->addWidget(NumButtonGroup->find(0xE), 4, 4);
	smallBtnLayout->addWidget(pbConstant[4], 4, 5);

	smallBtnLayout->addWidget(pbStat["ClearData"], 5, 0);
	smallBtnLayout->addWidget(pbScientific["LogNatural"], 5, 1);
	smallBtnLayout->addWidget(pbLogic["One-Complement"], 5, 2);
	smallBtnLayout->addWidget(pbPower, 5, 3);
	smallBtnLayout->addWidget(NumButtonGroup->find(0xF), 5, 4);
	smallBtnLayout->addWidget(pbConstant[5], 5, 5);

	smallBtnLayout->setRowStretch(0, 0);
	smallBtnLayout->setRowStretch(1, 0);
	smallBtnLayout->setRowStretch(2, 0);
	smallBtnLayout->setRowStretch(3, 0);
	smallBtnLayout->setRowStretch(4, 0);
	smallBtnLayout->setRowStretch(5, 0);

	// large button layout
	largeBtnLayout->addWidget(pbClear, 0, 0);
	largeBtnLayout->addWidget(pbAC, 0, 1);

	largeBtnLayout->addWidget(pbParenOpen, 1, 0);
	largeBtnLayout->addWidget(pbParenClose, 1, 1);

	largeBtnLayout->addWidget(pbMemRecall, 2, 0);
	largeBtnLayout->addWidget(pbMemStore, 2, 1);

	largeBtnLayout->addWidget(pbMemPlusMinus, 3, 0);
	largeBtnLayout->addWidget(pbMC, 3, 1);

	largeBtnLayout->addWidget(pbPercent, 4, 0);
	largeBtnLayout->addWidget(pbPlusMinus, 4, 1);

	// top layout
	topLayout->addWidget(pbAngleChoose);
	topLayout->addStretch();
	topLayout->addWidget(pbInv);
	// Windows 10/11 layout order: header -> display -> scientific controls -> memory row -> buttons
	mainLayout->insertWidget(0, mStandardPage);  // memory row at pos 0
	mainLayout->insertWidget(0, mScientificPage); // scientific controls pushed to pos 0
	mainLayout->insertWidget(0, mProgrammerPage); // programmer controls pushed to pos 0
	mainLayout->insertWidget(0, calc_display);    // display pushed to pos 0
	mainLayout->insertWidget(0, mHeaderWidget);   // header pushed to pos 0

	mFunctionButtonList.append(pbScientific["HypMode"]);
	mFunctionButtonList.append(pbInv);
	mFunctionButtonList.append(pbRoot);
	mFunctionButtonList.append(pbScientific["Sine"]);
	mFunctionButtonList.append(pbPlusMinus);
	mFunctionButtonList.append(pbScientific["Cosine"]);
	mFunctionButtonList.append(pbReci);
	mFunctionButtonList.append(pbScientific["Tangent"]);
	mFunctionButtonList.append(pbFactorial);
	mFunctionButtonList.append(pbScientific["Log10"]);
	mFunctionButtonList.append(pbSquare);
	mFunctionButtonList.append(pbScientific["LogNatural"]);
	mFunctionButtonList.append(pbPower);

	mMemButtonList.append(pbEE);
	mMemButtonList.append(pbMemRecall);
	mMemButtonList.append(pbMemPlusMinus);
	mMemButtonList.append(pbMemStore);
	mMemButtonList.append(pbMC);
	mMemButtonList.append(pbClear);
	mMemButtonList.append(pbAC);

	mOperationButtonList.append(pbX);
	mOperationButtonList.append(pbParenOpen);
	mOperationButtonList.append(pbParenClose);
	mOperationButtonList.append(pbLogic["AND"]);
	mOperationButtonList.append(pbDivision);
	mOperationButtonList.append(pbLogic["OR"]);
	mOperationButtonList.append(pbLogic["XOR"]);
	mOperationButtonList.append(pbPlus);
	mOperationButtonList.append(pbMinus);
	mOperationButtonList.append(pbLogic["LeftShift"]);
	mOperationButtonList.append(pbLogic["RightShift"]);
	mOperationButtonList.append(pbPeriod);
	mOperationButtonList.append(pbEqual);
	mOperationButtonList.append(pbPercent);
	mOperationButtonList.append(pbLogic["One-Complement"]);
	mOperationButtonList.append(pbMod);

	updateSettings();

	// Switch to decimal
	resetBase();
	slotAngleSelected(0);

	updateGeometry();

	resize(530, 590);
	// setFixedSize(sizeHint());

	UpdateDisplay(true);

	// Read and set button groups

	//actionStatshow->setChecked(false);
	slotStatshow(false);

	//actionScientificshow->setChecked(false);
	slotScientificshow(false);

	//actionLogicshow->setChecked(false);
	slotLogicshow(false);

	//actionConstantsShow->setChecked(false);
	slotConstantsShow(false);

	//statusBar()->hide();

	// Apply embedded icons to buttons
	applyIconToButton(pbPercent, percent_png, percent_png_len, TQString("Percent"));
	applyIconToButton(pbStdPercent, percent_png, percent_png_len, TQString("Percent"));
	
	applyIconToButton(pbSquare, square_png, square_png_len, TQString("Square"));
	applyIconToButton(pbStdSquare, square_png, square_png_len, TQString("Square"));
	
	applyIconToButton(pbRoot, squareroot_png, squareroot_png_len, TQString("Square Root"));
	applyIconToButton(pbStdRoot, squareroot_png, squareroot_png_len, TQString("Square Root"));
	
	applyIconToButton(pbDivision, divide_png, divide_png_len, TQString("Division"));
	applyIconToButton(pbStdDivision, divide_png, divide_png_len, TQString("Division"));
	
	applyIconToButton(pbPlus, plus_png, plus_png_len, TQString("Addition"));
	applyIconToButton(pbStdPlus, plus_png, plus_png_len, TQString("Addition"));
	
	applyIconToButton(pbMinus, minus_png, minus_png_len, TQString("Subtraction"));
	applyIconToButton(pbStdMinus, minus_png, minus_png_len, TQString("Subtraction"));
	
	applyIconToButton(pbX, multiply_png, multiply_png_len, TQString("Multiplication"));
	applyIconToButton(pbStdX, multiply_png, multiply_png_len, TQString("Multiplication"));
	
	applyIconToButton(pbStdBackSpace, backspace_png, backspace_png_len, TQString("Backspace"));
	
	applyIconToButton(pbReci, invert_png, invert_png_len, TQString("Reciprocal"));
	applyIconToButton(pbStdReci, invert_png, invert_png_len, TQString("Reciprocal"));
	
	applyIconToButton(pbPlusMinus, plusminus_png, plusminus_png_len, TQString("Change sign"));
	applyIconToButton(pbStdPlusMinus, plusminus_png, plusminus_png_len, TQString("Change sign"));
	
	applyIconToButton(pbEqual, equal_png, equal_png_len, TQString("Result"));
	applyIconToButton(pbStdEqual, equal_png, equal_png_len, TQString("Result"));

	updateModeVisibility();
	tqApp->installEventFilter(this);
}

Calculator::~Calculator()
{
	if (m_historyWindow) delete m_historyWindow;
	delete calc_display;
}

void Calculator::setupMainActions(void)
{
/*
	// file menu
	KStdAction::quit(this, TQ_SLOT(close()), actionCollection());

	// edit menu
	KStdAction::cut(calc_display, TQ_SLOT(slotCut()), actionCollection());
	KStdAction::copy(calc_display, TQ_SLOT(slotCopy()), actionCollection());
	KStdAction::paste(calc_display, TQ_SLOT(slotPaste()), actionCollection());

	// settings menu
	actionStatshow =  new TDEToggleAction(TQString("&Statistic Buttons"), 0,
					    actionCollection(), "show_stat");
	actionStatshow->setChecked(true);
	connect(actionStatshow, TQ_SIGNAL(toggled(bool)),
		TQ_SLOT(slotStatshow(bool)));

	actionScientificshow = new TDEToggleAction(TQString("Science/&Engineering Buttons"),
						 0, actionCollection(), "show_science");
	actionScientificshow->setChecked(true);
	connect(actionScientificshow, TQ_SIGNAL(toggled(bool)),
		TQ_SLOT(slotScientificshow(bool)));

	actionLogicshow = new TDEToggleAction(TQString("&Logic Buttons"), 0,
					    actionCollection(), "show_logic");
	actionLogicshow->setChecked(true);
	connect(actionLogicshow, TQ_SIGNAL(toggled(bool)),
		TQ_SLOT(slotLogicshow(bool)));

	actionConstantsShow = new TDEToggleAction(TQString("&Constants Buttons"), 0,
						actionCollection(), "show_constants");
	actionConstantsShow->setChecked(true);
	connect(actionConstantsShow, TQ_SIGNAL(toggled(bool)),
		TQ_SLOT(slotConstantsShow(bool)));


	(void) new TQAction(TQString("&Show All"), 0, this, TQ_SLOT(slotShowAll()),
			   actionCollection(), "show_all");

	(void) new TQAction(TQString("&Hide All"), 0, this, TQ_SLOT(slotHideAll()),
			   actionCollection(), "hide_all");

	KStdAction::preferences(this, TQ_SLOT(showSettings()), actionCollection());

	KStdAction::keyBindings(guiFactory(), TQ_SLOT(configureShortcuts()),
actionCollection());
*/
}

void Calculator::setupStatusbar(void)
{
	// Status bar contents
	//statusBar()->insertFixedItem(" NORM ", 0, true);
	//statusBar()->setItemAlignment(0, AlignCenter);

	//statusBar()->insertFixedItem(" HEX ", 1, true);
	//statusBar()->setItemAlignment(1, AlignCenter);

	//statusBar()->insertFixedItem(" DEG ", 2, true);
	//statusBar()->setItemAlignment(2, AlignCenter);

	//statusBar()->insertFixedItem(" \xa0\xa0 ", 3, true); // Memory indicator
	//statusBar()->setItemAlignment(3, AlignCenter);
}

TQWidget* Calculator::setupNumericKeys(TQWidget *parent)
{
	TQ_CHECK_PTR(mSmallPage);
	TQ_CHECK_PTR(mLargePage);

	TQWidget *thisPage = new TQWidget(parent);

	CalcButton *tmp_pb;

	NumButtonGroup = new TQButtonGroup(0, "Num-Button-Group");
	connect(NumButtonGroup, TQ_SIGNAL(clicked(int)),
		TQ_SLOT(slotNumberclicked(int)));

	tmp_pb = new CalcButton("0", thisPage, "0-Button");
	tmp_pb->setAccel(Key_0);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 0);

	tmp_pb = new CalcButton("1", thisPage, "1-Button");
	tmp_pb->setAccel(Key_1);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 1);

	tmp_pb = new CalcButton("2", thisPage, "2-Button");
	tmp_pb->setAccel(Key_2);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 2);

	tmp_pb = new CalcButton("3", thisPage, "3-Button");
	tmp_pb->setAccel(Key_3);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 3);

	tmp_pb = new CalcButton("4", thisPage, "4-Button");
	tmp_pb->setAccel(Key_4);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 4);

	tmp_pb = new CalcButton("5", thisPage, "5-Button");
	tmp_pb->setAccel(Key_5);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 5);

	tmp_pb = new CalcButton("6", thisPage, "6-Button");
	tmp_pb->setAccel(Key_6);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 6);

	tmp_pb = new CalcButton("7", thisPage, "7-Button");
	tmp_pb->setAccel(Key_7);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 7);

	tmp_pb = new CalcButton("8", thisPage, "8-Button");
	tmp_pb->setAccel(Key_8);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 8);

	tmp_pb = new CalcButton("9", thisPage, "9-Button");
	tmp_pb->setAccel(Key_9);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 9);

	pbEE = new CalcButton(thisPage, "EE-Button");
	pbEE->addMode(ModeNormal, "x<small>" "\xb7" "10</small><sup>y</sup>",
			TQString("Exponent"), true);
	pbEE->setAccel(Key_E);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbEE, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbEE, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotEEclicked(void)));

	pbParenClose = new CalcButton(")", mLargePage, "ParenClose-Button");
	pbParenClose->setAccel(Key_ParenRight);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbParenClose, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbParenClose,TQ_SIGNAL(clicked(void)),TQ_SLOT(slotParenCloseclicked(void)));

	pbX = new CalcButton("X", thisPage, "Multiply-Button", TQString("Multiplication"));
	pbX->setAccel(Key_multiply);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbX, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	//accel()->insert("Pressed '*'", TQString("Pressed Multiplication-Button"),
	//		0, Key_Asterisk, pbX, TQ_SLOT(animateClick()));
	connect(pbX, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotXclicked(void)));

	pbDivision = new CalcButton("/", thisPage, "Division-Button", TQString("Division"));
	pbDivision->setAccel(Key_Slash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbDivision, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbDivision, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotDivisionclicked(void)));

	pbPlus = new CalcButton("+", thisPage, "Plus-Button", TQString("Addition"));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbPlus, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	pbPlus->setAccel(Key_Plus);
	connect(pbPlus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusclicked(void)));

	pbMinus = new CalcButton("-", thisPage, "Minus-Button", TQString("Subtraction"));
	pbMinus->setAccel(Key_Minus);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbMinus, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbMinus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMinusclicked(void)));

	pbPeriod = new CalcButton(TQString("."), thisPage,
					"Period-Button", TQString("Decimal point"));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbPeriod, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	//accel()->insert("Decimal Point (Period)", TQString("Pressed Decimal Point"),
	//		0, Key_Period, pbPeriod, TQ_SLOT(animateClick()));
	//accel()->insert("Decimal Point (Comma)", TQString("Pressed Decimal Point"),
	//		0, Key_Comma, pbPeriod, TQ_SLOT(animateClick()));
	connect(pbPeriod, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPeriodclicked(void)));

	pbEqual = new CalcButton("=", thisPage, "Equal-Button", TQString("Result"));
	pbEqual->setAccel(Key_Enter);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbEqual, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	//accel()->insert("Entered Equal", TQString("Pressed Equal-Button"),
	//		0, Key_Equal, pbEqual, TQ_SLOT(animateClick()));
	//accel()->insert("Entered Return", TQString("Pressed Equal-Button"),
	//		0, Key_Return, pbEqual, TQ_SLOT(animateClick()));
	connect(pbEqual, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotEqualclicked(void)));


	TQGridLayout *thisLayout = new TQGridLayout(thisPage, 5, 4, 0,
						  mInternalSpacing);

	// large button layout
	thisLayout->addWidget(pbEE, 0, 0);
	thisLayout->addWidget(pbDivision, 0, 1);
	thisLayout->addWidget(pbX, 0, 2);
	thisLayout->addWidget(pbMinus, 0, 3);

	thisLayout->addWidget(NumButtonGroup->find(7), 1, 0);
	thisLayout->addWidget(NumButtonGroup->find(8), 1, 1);
	thisLayout->addWidget(NumButtonGroup->find(9), 1, 2);
	thisLayout->addMultiCellWidget(pbPlus, 1, 2, 3, 3);

	thisLayout->addWidget(NumButtonGroup->find(4), 2, 0);
	thisLayout->addWidget(NumButtonGroup->find(5), 2, 1);
	thisLayout->addWidget(NumButtonGroup->find(6), 2, 2);
	//thisLayout->addMultiCellWidget(pbPlus, 1, 2, 3, 3);

	thisLayout->addWidget(NumButtonGroup->find(1), 3, 0);
	thisLayout->addWidget(NumButtonGroup->find(2), 3, 1);
	thisLayout->addWidget(NumButtonGroup->find(3), 3, 2);
	thisLayout->addMultiCellWidget(pbEqual, 3, 4, 3, 3);

	thisLayout->addMultiCellWidget(NumButtonGroup->find(0), 4, 4, 0, 1);
	thisLayout->addWidget(pbPeriod, 4, 2);
	//thisLayout->addMultiCellWidget(pbEqual, 3, 4, 3, 3);

	thisLayout->addColSpacing(0,10);
	thisLayout->addColSpacing(1,10);
	thisLayout->addColSpacing(2,10);
	thisLayout->addColSpacing(3,10);
	thisLayout->addColSpacing(4,10);


	pbMemRecall = new CalcButton("MR", mLargePage, "MemRecall-Button", TQString("Memory recall"));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbMemRecall, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbMemRecall, TQ_SIGNAL(clicked(void)),
		TQ_SLOT(slotMemRecallclicked(void)));
        pbMemRecall->setDisabled(true); // At start, there is nothing in memory

	pbMemPlusMinus = new CalcButton(mLargePage, "MPlusMinus-Button");
	pbMemPlusMinus->addMode(ModeNormal, "M+", TQString("Add display to memory"));
	pbMemPlusMinus->addMode(ModeInverse, "M-", TQString("Subtract from memory"));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		pbMemPlusMinus, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbMemPlusMinus, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbMemPlusMinus,TQ_SIGNAL(clicked(void)),
		TQ_SLOT(slotMemPlusMinusclicked(void)));

	pbMemStore = new CalcButton("MS", mLargePage, "MemStore-Button",
					TQString("Memory store"));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbMemStore, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbMemStore, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMemStoreclicked(void)));


	pbMC = new CalcButton("MC", mLargePage, "MemClear-Button", TQString("Clear memory"));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbMC, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbMC, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMCclicked(void)));

	pbClear = new CalcButton("C", mLargePage, "Clear-Button", TQString("Clear"));
	pbClear->setAccel(Key_Prior);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbClear, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	//accel()->insert("Entered 'ESC'", TQString("Pressed ESC-Button"), 0,
	//		Key_Escape, pbClear, TQ_SLOT(animateClick()));
	connect(pbClear, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotClearclicked(void)));

	pbAC = new CalcButton("AC", mLargePage, "AC-Button", TQString("Clear all"));
	pbAC->setAccel(0);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbAC, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbAC, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotACclicked(void)));

	pbParenOpen = new CalcButton("(", mLargePage, "ParenOpen-Button");
	pbParenOpen->setAccel(Key_ParenLeft);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbParenOpen, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbParenOpen, TQ_SIGNAL(clicked(void)),TQ_SLOT(slotParenOpenclicked(void)));

	pbPercent = new CalcButton("%", mLargePage, "Percent-Button", TQString("Percent"));
	pbPercent->setAccel(Key_Percent);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbPercent, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbPercent, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPercentclicked(void)));

	pbPlusMinus = new CalcButton("\xb1", mLargePage, "Sign-Button", TQString("Change sign"));
	pbPlusMinus->setAccel(Key_Backslash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbPlusMinus, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbPlusMinus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusMinusclicked(void)));


	tmp_pb = new CalcButton("A", mSmallPage, "A-Button");
	tmp_pb->setAccel(Key_A);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 0xA);

	tmp_pb = new CalcButton("B", mSmallPage, "B-Button");
	tmp_pb->setAccel(Key_B);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 0xB);

	tmp_pb = new CalcButton("C", mSmallPage, "C-Button");
	tmp_pb->setAccel(Key_C);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 0xC);

	tmp_pb = new CalcButton("D", mSmallPage, "D-Button");
	tmp_pb->setAccel(Key_D);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 0xD);

	tmp_pb = new CalcButton("E", mSmallPage, "E-Button");
	tmp_pb->setAccel(Key_E);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 0xE);

	tmp_pb = new CalcButton("F", mSmallPage, "F-Button");
	tmp_pb->setAccel(Key_F);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
        NumButtonGroup->insert(tmp_pb, 0xF);

	return thisPage;
}

void Calculator::setupLogicKeys(TQWidget *parent)
{
	TQ_CHECK_PTR(parent);

	CalcButton *tmp_pb;

	tmp_pb = new CalcButton("AND", parent, "AND-Button", TQString("Bitwise AND"));
	pbLogic.insert("AND", tmp_pb);
	tmp_pb->setAccel(Key_Ampersand);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotANDclicked(void)));

	tmp_pb = new CalcButton("OR", parent, "OR-Button", TQString("Bitwise OR"));
	pbLogic.insert("OR", tmp_pb);
	tmp_pb->setAccel(Key_Bar);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotORclicked(void)));

	tmp_pb = new CalcButton("XOR", parent, "XOR-Button", TQString("Bitwise XOR"));
	pbLogic.insert("XOR", tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotXORclicked(void)));

	tmp_pb = new CalcButton("Cmp", parent, "One-Complement-Button",
					TQString("One's complement"));
	pbLogic.insert("One-Complement", tmp_pb);
	tmp_pb->setAccel(Key_AsciiTilde);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotNegateclicked(void)));

	tmp_pb = new CalcButton("Lsh", parent, "LeftBitShift-Button",
					TQString("Left bit shift"));
	tmp_pb->setAccel(Key_Less);
	pbLogic.insert("LeftShift", tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)),
		TQ_SLOT(slotLeftShiftclicked(void)));

	tmp_pb = new CalcButton("Rsh", parent, "RightBitShift-Button",
					TQString("Right bit shift"));
	tmp_pb->setAccel(Key_Greater);
	pbLogic.insert("RightShift", tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)),
		TQ_SLOT(slotRightShiftclicked(void)));
}

void Calculator::setupScientificKeys(TQWidget *parent)
{
	TQ_CHECK_PTR(parent);

	CalcButton *tmp_pb;

	tmp_pb = new CalcButton("Hyp", parent, "Hyp-Button", TQString("Hyperbolic mode"));
	pbScientific.insert("HypMode", tmp_pb);
	tmp_pb->setAccel(Key_H);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotHyptoggled(bool)));
	tmp_pb->setToggleButton(true);

	tmp_pb = new CalcButton(parent, "Sin-Button");
	pbScientific.insert("Sine", tmp_pb);
	tmp_pb->addMode(ModeNormal, "Sin", TQString("Sine"));
	tmp_pb->addMode(ModeInverse, "Asin", TQString("Arc sine"));
	tmp_pb->addMode(ModeHyperbolic, "Sinh", TQString("Hyperbolic sine"));
	tmp_pb->addMode(ButtonModeFlags(ModeInverse | ModeHyperbolic),
			"Asinh", TQString("Inverse hyperbolic sine"));
	tmp_pb->setAccel(Key_S);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotSinclicked(void)));

	tmp_pb = new CalcButton(parent, "Cos-Button");
	pbScientific.insert("Cosine", tmp_pb);
	tmp_pb->addMode(ModeNormal, "Cos", TQString("Cosine"));
	tmp_pb->addMode(ModeInverse, "Acos", TQString("Arc cosine"));
	tmp_pb->addMode(ModeHyperbolic, "Cosh", TQString("Hyperbolic cosine"));
	tmp_pb->addMode(ButtonModeFlags(ModeInverse | ModeHyperbolic),
			"Acosh", TQString("Inverse hyperbolic cosine"));
	tmp_pb->setAccel(Key_C);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotCosclicked(void)));

	tmp_pb = new CalcButton(parent, "Tan-Button");
	pbScientific.insert("Tangent", tmp_pb);
	tmp_pb->addMode(ModeNormal, "Tan", TQString("Tangent"));
	tmp_pb->addMode(ModeInverse, "Atan", TQString("Arc tangent"));
	tmp_pb->addMode(ModeHyperbolic, "Tanh", TQString("Hyperbolic tangent"));
	tmp_pb->addMode(ButtonModeFlags(ModeInverse | ModeHyperbolic),
			"Atanh", TQString("Inverse hyperbolic tangent"));
	tmp_pb->setAccel(Key_T);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)),TQ_SLOT(slotTanclicked(void)));

	tmp_pb = new CalcButton(parent, "Ln-Button");
	tmp_pb->addMode(ModeNormal, "Ln", TQString("Natural log"));
	tmp_pb->addMode(ModeInverse, "e<sup> x </sup>", TQString("Exponential function"),
			true);
	pbScientific.insert("LogNatural", tmp_pb);
	tmp_pb->setAccel(Key_N);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotLnclicked(void)));

	tmp_pb = new CalcButton(parent, "Log-Button");
	tmp_pb->addMode(ModeNormal, "Log", TQString("Logarithm to base 10"));
	tmp_pb->addMode(ModeInverse, "10<sup> x </sup>", TQString("10 to the power of x"),
			true);
	pbScientific.insert("Log10", tmp_pb);
	tmp_pb->setAccel(Key_L);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotLogclicked(void)));

}

void Calculator::setupStatisticKeys(TQWidget *parent)
{
	TQ_CHECK_PTR(parent);

	CalcButton *tmp_pb;

	tmp_pb = new CalcButton(parent, "Stat.NumData-Button");
	tmp_pb->addMode(ModeNormal, "N", TQString("Number of data entered"));
	tmp_pb->addMode(ModeInverse, TQString::fromUtf8("\xce\xa3")
			+ "x", TQString("Sum of all data items"));
	pbStat.insert("NumData", tmp_pb);
        mStatButtonList.append(tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStatNumclicked(void)));

	tmp_pb = new CalcButton(parent, "Stat.Median-Button");
	tmp_pb->addMode(ModeNormal, "Med", TQString("Median"));
	pbStat.insert("Median", tmp_pb);
        mStatButtonList.append(tmp_pb);
	//TQToolTip::add(tmp_pb, TQString("Median"));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStatMedianclicked(void)));

	tmp_pb = new CalcButton(parent, "Stat.Mean-Button");
	tmp_pb->addMode(ModeNormal, "Mea", TQString("Mean"));
	tmp_pb->addMode(ModeInverse, TQString::fromUtf8("\xce\xa3")
			+ "x<sup>2</sup>",
			TQString("Sum of all data items squared"), true);
	pbStat.insert("Mean", tmp_pb);
        mStatButtonList.append(tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStatMeanclicked(void)));

	tmp_pb = new CalcButton(parent, "Stat.StandardDeviation-Button");
	tmp_pb->addMode(ModeNormal, TQString::fromUtf8("σ",-1) +  "<sub>N-1</sub>",
			TQString("Sample standard deviation"), true);
	tmp_pb->addMode(ModeInverse, TQString::fromUtf8("σ",-1) +  "<sub>N</sub>",
			TQString("Standard deviation"), true);
	pbStat.insert("StandardDeviation", tmp_pb);
        mStatButtonList.append(tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStatStdDevclicked(void)));

	tmp_pb = new CalcButton(parent, "Stat.DataInput-Button");
	tmp_pb->addMode(ModeNormal, "Dat", TQString("Enter data"));
	tmp_pb->addMode(ModeInverse, "CDat", TQString("Delete last data item"));
	pbStat.insert("InputData", tmp_pb);
        mStatButtonList.append(tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStatDataInputclicked(void)));

	tmp_pb = new CalcButton(parent, "Stat.ClearData-Button");
	tmp_pb->addMode(ModeNormal, "CSt", TQString("Clear data store"));
	pbStat.insert("ClearData", tmp_pb);
        mStatButtonList.append(tmp_pb);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(tmp_pb, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStatClearDataclicked(void)));
}

void Calculator::setupConstantsKeys(TQWidget *parent)
{
	TQ_CHECK_PTR(parent);

	ConstButtonGroup = new TQButtonGroup(0, "Const-Button-Group");
	connect(ConstButtonGroup, TQ_SIGNAL(clicked(int)), TQ_SLOT(slotConstclicked(int)));


	CalcConstButton *tmp_pb;
	tmp_pb = new CalcConstButton(parent, 0, "C1");
	tmp_pb->setAccel(ALT + Key_1);
	pbConstant[0] = tmp_pb;
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	ConstButtonGroup->insert(tmp_pb, 0);

	tmp_pb = new CalcConstButton(parent, 1, "C2");
        tmp_pb->setAccel(ALT + Key_2);
	pbConstant[1] = tmp_pb;
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
                tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	ConstButtonGroup->insert(tmp_pb, 1);

	tmp_pb = new CalcConstButton(parent, 2, "C3");
        tmp_pb->setAccel(ALT + Key_3);
	pbConstant[2] = tmp_pb;
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
                tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	ConstButtonGroup->insert(tmp_pb, 2);

	tmp_pb = new CalcConstButton(parent, 3, "C4");
        tmp_pb->setAccel(ALT + Key_4);
	pbConstant[3] = tmp_pb;
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
                tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	ConstButtonGroup->insert(tmp_pb, 3);

	tmp_pb = new CalcConstButton(parent, 4, "C5");
        tmp_pb->setAccel(ALT + Key_5);
	pbConstant[4] = tmp_pb;
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
                tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	ConstButtonGroup->insert(tmp_pb, 4);

	tmp_pb = new CalcConstButton(parent, 5, "C6");
        tmp_pb->setAccel(ALT + Key_6);
	pbConstant[5]  = tmp_pb;
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
                tmp_pb, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		tmp_pb, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	ConstButtonGroup->insert(tmp_pb, 5);

	changeButtonNames();

	// add menu with scientific constants
	//CalcConstMenu *tmp_menu = new CalcConstMenu(this);
	//menuBar()->insertItem(TQString("&Constants"), tmp_menu, -1, 2);
	//connect(tmp_menu, TQ_SIGNAL(activated(int)), this,
	//	TQ_SLOT(slotConstantToDisplay(int)));
}

void Calculator::setupHeaderBar(TQWidget *parent)
{
	mHeaderWidget = new TQWidget(parent);
	TQHBoxLayout *headerLayout = new TQHBoxLayout(mHeaderWidget, 0, 4);

	// Hamburger menu button
	pbHamburger = new CalcButton(mHeaderWidget, "Hamburger-Button");
	applyIconToButton(pbHamburger, menu_png, menu_png_len, TQString("Menu"));
	pbHamburger->setMinimumWidth(36);
	pbHamburger->setMaximumWidth(36);
	pbHamburger->setMinimumHeight(28);
	pbHamburger->setMaximumHeight(28);
	pbHamburger->setButtonType(CalcButton::TypeHeader);
	connect(pbHamburger, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotHamburgerClicked(void)));
	headerLayout->addWidget(pbHamburger);

	// Mode title label
	lblModeTitle = new TQLabel(TQString("Standard"), mHeaderWidget);
	TQFont titleFont = lblModeTitle->font();
	titleFont.setBold(true); // Make bold
	titleFont.setPointSize(titleFont.pointSize() + 2); // Increase size slightly
	lblModeTitle->setFont(titleFont);
	headerLayout->addWidget(lblModeTitle);

	// Pin button
	pbPin = new CalcButton(mHeaderWidget, "Pin-Button");
	applyIconToButton(pbPin, pin_png, pin_png_len, TQString("Keep Above"));
	pbPin->setMinimumWidth(28);
	pbPin->setMaximumWidth(28);
	pbPin->setMinimumHeight(28);
	pbPin->setMaximumHeight(28);
	pbPin->setButtonType(CalcButton::TypeHeader);
	connect(pbPin, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPinClicked(void)));

	headerLayout->addStretch();
	headerLayout->addWidget(pbPin);

	// Constants menu button
	pbConstantsMenu = new CalcButton(mHeaderWidget, "Constants-Menu-Button");
	applyIconToButton(pbConstantsMenu, const_png, const_png_len, TQString("Constants"));
	pbConstantsMenu->setMinimumWidth(36);
	pbConstantsMenu->setMaximumWidth(36);
	pbConstantsMenu->setMinimumHeight(28);
	pbConstantsMenu->setMaximumHeight(28);
	pbConstantsMenu->setButtonType(CalcButton::TypeHeader);
	
	CalcConstMenu *const_menu = new CalcConstMenu(this);
	pbConstantsMenu->setPopup(const_menu);
	connect(const_menu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotConstantToDisplay(int)));
	headerLayout->addWidget(pbConstantsMenu);

	// History button
	pbHistory = new CalcButton(mHeaderWidget, "History-Button");
	applyIconToButton(pbHistory, history_png, history_png_len, TQString("History"));
	pbHistory->setMinimumWidth(36);
	pbHistory->setMaximumWidth(36);
	pbHistory->setMinimumHeight(28);
	pbHistory->setMaximumHeight(28);
	pbHistory->setButtonType(CalcButton::TypeHeader);
	connect(pbHistory, TQ_SIGNAL(clicked(void)), this, TQ_SLOT(slotHistoryClicked(void)));
	headerLayout->addWidget(pbHistory);

	// Minimize button
	pbMinimize = new CalcButton(mHeaderWidget, "Minimize-Button");
	applyIconToButton(pbMinimize, minimize_png, minimize_png_len, TQString("Minimize"));
	pbMinimize->setMinimumWidth(36);
	pbMinimize->setMaximumWidth(36);
	pbMinimize->setMinimumHeight(28);
	pbMinimize->setMaximumHeight(28);
	pbMinimize->setButtonType(CalcButton::TypeHeader);
	connect(pbMinimize, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMinimizeClicked(void)));
	headerLayout->addWidget(pbMinimize);

	// Maximize button
	pbMaximize = new CalcButton(mHeaderWidget, "Maximize-Button");
	applyIconToButton(pbMaximize, maximize_png, maximize_png_len, TQString("Maximize"));
	pbMaximize->setMinimumWidth(36);
	pbMaximize->setMaximumWidth(36);
	pbMaximize->setMinimumHeight(28);
	pbMaximize->setMaximumHeight(28);
	pbMaximize->setButtonType(CalcButton::TypeHeader);
	connect(pbMaximize, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMaximizeClicked(void)));
	headerLayout->addWidget(pbMaximize);

	// Close button
	pbClose = new CalcButton(mHeaderWidget, "Close-Button");
	applyIconToButton(pbClose, close_png, close_png_len, TQString("Close"));
	pbClose->setMinimumWidth(36);
	pbClose->setMaximumWidth(36);
	pbClose->setMinimumHeight(28);
	pbClose->setMaximumHeight(28);
	pbClose->setButtonType(CalcButton::TypeHeader);
	connect(pbClose, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotCloseClicked(void)));
	headerLayout->addWidget(pbClose);

	// Hide by default
	pbMinimize->hide();
	pbMaximize->hide();
	pbClose->hide();

	mHeaderWidget->installEventFilter(this);
	lblModeTitle->installEventFilter(this);
}

void Calculator::setupMenuBar()
{
	displayMenu = new TQPopupMenu(this);
	connect(displayMenu, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotDisplayMenuActivated(int)));
	connect(displayMenu, TQ_SIGNAL(aboutToShow()), this, TQ_SLOT(slotUpdateDisplayMenu()));

	settingsMenu = new TQPopupMenu(this);

	menuBar()->insertItem("", displayMenu, 10);
	menuBar()->insertItem("", settingsMenu, 11);

	retranslateUi();
}

void Calculator::slotUpdateDisplayMenu()
{
	if (!displayMenu) return;
	
	int ids[] = {
		100, 101, 102, 103,
		200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212
	};
	CalcMode modes[] = {
		ModeStandard, ModeScientific, ModeProgrammer, ModeDateCalc,
		ModeVolume, ModeLength, ModeMass, ModeTemp, ModeEnergy, ModeArea, ModeSpeed, ModeTime, ModePower, ModeData, ModePressure, ModeAngle, ModeRoman
	};
	
	for (int i = 0; i < 17; ++i) {
		TQMenuItem *mi = displayMenu->findItem(ids[i]);
		if (mi && mi->custom()) {
			CalcMenuItem *cmi = (CalcMenuItem*)(mi->custom());
			cmi->setChecked(_calc_mode == modes[i]);
		}
	}
}

void Calculator::slotDisplayMenuActivated(int id)
{
	if (id == 100) {
		slotMenuStandard();
	} else if (id == 101) {
		slotMenuScientific();
	} else if (id == 102) {
		slotMenuProgrammer();
	} else if (id == 103) {
		slotMenuDateCalc();
	} else if (id >= 200 && id <= 212) {
		CalcMode conversionModes[] = {
			ModeVolume,
			ModeLength,
			ModeMass,
			ModeTemp,
			ModeEnergy,
			ModeArea,
			ModeSpeed,
			ModeTime,
			ModePower,
			ModeData,
			ModePressure,
			ModeAngle,
			ModeRoman
		};
		_calc_mode = conversionModes[id - 200];
		resetBase();
		updateModeVisibility();
	}
}

void Calculator::slotMenuStandard()
{
	_calc_mode = ModeStandard;
	resetBase();
	updateModeVisibility();
}

void Calculator::slotMenuScientific()
{
	_calc_mode = ModeScientific;
	calc_display->setStatusText(2, "Deg");
	slotAngleSelected(0);
	resetBase();
	updateModeVisibility();
}

void Calculator::slotMenuProgrammer()
{
	_calc_mode = ModeProgrammer;
	calc_display->setStatusText(1, "Hex");
	resetBase();
	BaseChooseGroup->show();
	for (int i=10; i<16; i++)
		(NumButtonGroup->find(i))->show();
	updateModeVisibility();
}

void Calculator::slotMenuDateCalc()
{
	_calc_mode = ModeDateCalc;
	resetBase();
	updateModeVisibility();
}

void Calculator::slotMenuVolume()
{
	_calc_mode = ModeVolume;
	resetBase();
	updateModeVisibility();
}

void Calculator::slotMenuConfigure()
{
	showSettings();
}

void Calculator::slotMenuAbout()
{
	SettingsDialog *dlg = new SettingsDialog(true, this);
	dlg->exec();
	delete dlg;
}

TQWidget* Calculator::setupStandardKeys(TQWidget *parent)
{
	// 1. Create the Memory Row
	TQWidget *memRow = new TQWidget(parent);
	TQHBoxLayout *memLayout = new TQHBoxLayout(memRow, 0, 2);

	pbStdMC = new CalcButton("MC", memRow, "StdMC-Button", TQString("Clear memory"));
	pbStdMC->setButtonType(CalcButton::TypeMemory);
	pbStdMC->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdMC, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMCclicked(void)));
	pbStdMC->setEnabled(false);
	memLayout->addWidget(pbStdMC);

	pbStdMR = new CalcButton("MR", memRow, "StdMR-Button", TQString("Memory recall"));
	pbStdMR->setButtonType(CalcButton::TypeMemory);
	pbStdMR->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdMR, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMemRecallclicked(void)));
	pbStdMR->setEnabled(false);
	memLayout->addWidget(pbStdMR);

	pbStdMPlus = new CalcButton("M+", memRow, "StdMPlus-Button", TQString("Add to memory"));
	pbStdMPlus->setButtonType(CalcButton::TypeMemory);
	pbStdMPlus->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdMPlus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStdMPlusclicked(void)));
	memLayout->addWidget(pbStdMPlus);

	pbStdMMinus = new CalcButton("M-", memRow, "StdMMinus-Button", TQString("Subtract from memory"));
	pbStdMMinus->setButtonType(CalcButton::TypeMemory);
	pbStdMMinus->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdMMinus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotStdMMinusclicked(void)));
	memLayout->addWidget(pbStdMMinus);

	pbStdMS = new CalcButton("MS", memRow, "StdMS-Button", TQString("Memory store"));
	pbStdMS->setButtonType(CalcButton::TypeMemory);
	pbStdMS->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdMS, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMemStoreclicked(void)));
	memLayout->addWidget(pbStdMS);

	mStandardPage = memRow;
	mStandardPage->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);
	mStandardPage->setMinimumHeight(30);

	// 2. Create the 4x6 Grid
	mStandardPageGrid = new TQWidget(parent);
	mStandardPageGrid->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	TQGridLayout *grid = new TQGridLayout(mStandardPageGrid, 6, 4, 0, 2);
	for (int i=0; i<6; i++) grid->setRowStretch(i, 1);
	for (int i=0; i<4; i++) grid->setColStretch(i, 1);

	// Digits 0-9
	StdNumButtonGroup = new TQButtonGroup(0, "Std-Num-Button-Group");
	connect(StdNumButtonGroup, TQ_SIGNAL(clicked(int)), TQ_SLOT(slotNumberclicked(int)));

	for (int i = 0; i < 10; ++i) {
		TQString name;
		name.sprintf("%d-StdButton", i);
		CalcButton *btn = new CalcButton(TQString::number(i), mStandardPageGrid, name);
		btn->setButtonType(CalcButton::TypeDigit);
		btn->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
		btn->setAccel(Key_0 + i);
		connect(this, TQ_SIGNAL(switchShowAccels(bool)), btn, TQ_SLOT(slotSetAccelDisplayMode(bool)));
		StdNumButtonGroup->insert(btn, i);
	}

	// Operator buttons
	pbStdPercent = new CalcButton("%", mStandardPageGrid, "StdPercent-Button", TQString("Percent"));
	pbStdPercent->setButtonType(CalcButton::TypeOperator);
	pbStdPercent->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdPercent->setAccel(Key_Percent);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdPercent, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdPercent, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPercentclicked(void)));

	pbStdRoot = new CalcButton("\xe2\x88\x9a", mStandardPageGrid, "StdRoot-Button", TQString("Square root"));
	pbStdRoot->setButtonType(CalcButton::TypeOperator);
	pbStdRoot->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdRoot, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotRootclicked(void)));

	pbStdSquare = new CalcButton(mStandardPageGrid, "StdSquare-Button");
	pbStdSquare->addMode(ModeNormal, "x<sup>2</sup>", TQString("Square"), true);
	pbStdSquare->setButtonType(CalcButton::TypeOperator);
	pbStdSquare->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdSquare, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotSquareclicked(void)));

	pbStdReci = new CalcButton(mStandardPageGrid, "StdReci-Button");
	pbStdReci->addMode(ModeNormal, "1/x", TQString("Reciprocal"));
	pbStdReci->setButtonType(CalcButton::TypeOperator);
	pbStdReci->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdReci->setAccel(Key_R);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdReci, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdReci, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotReciclicked(void)));

	pbStdCE = new CalcButton("CE", mStandardPageGrid, "StdCE-Button", TQString("Clear Entry"));
	pbStdCE->setButtonType(CalcButton::TypeOperator);
	pbStdCE->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	connect(pbStdCE, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotClearclicked(void)));

	pbStdC = new CalcButton("C", mStandardPageGrid, "StdC-Button", TQString("Clear"));
	pbStdC->setButtonType(CalcButton::TypeOperator);
	pbStdC->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdC->setAccel(0);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdC, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdC, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotACclicked(void)));

	pbStdBackSpace = new CalcButton("\xe2\x8c\xab", mStandardPageGrid, "StdBackSpace-Button", TQString("Backspace"));
	pbStdBackSpace->setButtonType(CalcButton::TypeOperator);
	pbStdBackSpace->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdBackSpace->setAccel(Key_Backspace);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdBackSpace, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdBackSpace, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotBackSpaceclicked(void)));

	pbStdDivision = new CalcButton("\xc3\xb7", mStandardPageGrid, "StdDivision-Button", TQString("Division"));
	pbStdDivision->setButtonType(CalcButton::TypeOperator);
	pbStdDivision->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdDivision->setAccel(Key_Slash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdDivision, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdDivision, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotDivisionclicked(void)));

	pbStdX = new CalcButton("\xc3\x97", mStandardPageGrid, "StdMultiply-Button", TQString("Multiplication"));
	pbStdX->setButtonType(CalcButton::TypeOperator);
	pbStdX->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdX->setAccel(Key_Asterisk);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdX, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdX, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotXclicked(void)));

	pbStdMinus = new CalcButton("-", mStandardPageGrid, "StdMinus-Button", TQString("Subtraction"));
	pbStdMinus->setButtonType(CalcButton::TypeOperator);
	pbStdMinus->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdMinus->setAccel(Key_Minus);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdMinus, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdMinus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMinusclicked(void)));

	pbStdPlus = new CalcButton("+", mStandardPageGrid, "StdPlus-Button", TQString("Addition"));
	pbStdPlus->setButtonType(CalcButton::TypeOperator);
	pbStdPlus->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdPlus->setAccel(Key_Plus);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdPlus, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdPlus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusclicked(void)));

	pbStdEqual = new CalcButton("=", mStandardPageGrid, "StdEqual-Button", TQString("Result"));
	pbStdEqual->setButtonType(CalcButton::TypeEqual);
	pbStdEqual->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdEqual->setAccel(Key_Enter);
	//accel()->insert("StdEntered Equal", TQString("Pressed Equal-Button"), 0, Key_Equal, pbStdEqual, TQ_SLOT(animateClick()));
	//accel()->insert("StdEntered Return", TQString("Pressed Equal-Button"), 0, Key_Return, pbStdEqual, TQ_SLOT(animateClick()));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdEqual, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdEqual, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotEqualclicked(void)));

	pbStdPlusMinus = new CalcButton("\xc2\xb1", mStandardPageGrid, "StdPlusMinus-Button", TQString("Change sign"));
	pbStdPlusMinus->setButtonType(CalcButton::TypeOperator);
	pbStdPlusMinus->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbStdPlusMinus->setAccel(Key_Backslash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdPlusMinus, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdPlusMinus, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusMinusclicked(void)));

	pbStdPeriod = new CalcButton(TQString("."), mStandardPageGrid, "StdPeriod-Button", TQString("Decimal point"));
	pbStdPeriod->setButtonType(CalcButton::TypeOperator);
	pbStdPeriod->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	//accel()->insert("StdDecimal Point (Period)", TQString("Pressed Decimal Point"), 0, Key_Period, pbStdPeriod, TQ_SLOT(animateClick()));
	//accel()->insert("StdDecimal Point (Comma)", TQString("Pressed Decimal Point"), 0, Key_Comma, pbStdPeriod, TQ_SLOT(animateClick()));
	connect(this, TQ_SIGNAL(switchShowAccels(bool)), pbStdPeriod, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(pbStdPeriod, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPeriodclicked(void)));

	// Assemble Grid
	// Row 0
	grid->addWidget(pbStdPercent, 0, 0);
	grid->addWidget(pbStdRoot, 0, 1);
	grid->addWidget(pbStdSquare, 0, 2);
	grid->addWidget(pbStdReci, 0, 3);

	// Row 1
	grid->addWidget(pbStdCE, 1, 0);
	grid->addWidget(pbStdC, 1, 1);
	grid->addWidget(pbStdBackSpace, 1, 2);
	grid->addWidget(pbStdDivision, 1, 3);

	// Row 2
	grid->addWidget(StdNumButtonGroup->find(7), 2, 0);
	grid->addWidget(StdNumButtonGroup->find(8), 2, 1);
	grid->addWidget(StdNumButtonGroup->find(9), 2, 2);
	grid->addWidget(pbStdX, 2, 3);

	// Row 3
	grid->addWidget(StdNumButtonGroup->find(4), 3, 0);
	grid->addWidget(StdNumButtonGroup->find(5), 3, 1);
	grid->addWidget(StdNumButtonGroup->find(6), 3, 2);
	grid->addWidget(pbStdMinus, 3, 3);

	// Row 4
	grid->addWidget(StdNumButtonGroup->find(1), 4, 0);
	grid->addWidget(StdNumButtonGroup->find(2), 4, 1);
	grid->addWidget(StdNumButtonGroup->find(3), 4, 2);
	grid->addWidget(pbStdPlus, 4, 3);

	// Row 5
	grid->addWidget(pbStdPlusMinus, 5, 0);
	grid->addWidget(StdNumButtonGroup->find(0), 5, 1);
	grid->addWidget(pbStdPeriod, 5, 2);
	grid->addWidget(pbStdEqual, 5, 3);

	return memRow;
}

void Calculator::slotPinClicked(void)
{
	is_pinned = !is_pinned;
	if (is_pinned) {
		KWin::setState(winId(), NET::KeepAbove);
		applyIconToButton(pbPin, unpin_png, unpin_png_len, TQString("Normal Window"));
	} else {
		KWin::clearState(winId(), NET::KeepAbove);
		applyIconToButton(pbPin, pin_png, pin_png_len, TQString("Keep Above"));
	}
}

void Calculator::slotMinimizeClicked()
{
	showMinimized();
}

void Calculator::slotMaximizeClicked()
{
	if (isMaximized()) {
		showNormal();
		applyIconToButton(pbMaximize, maximize_png, maximize_png_len, TQString("Maximize"));
	} else {
		showMaximized();
		applyIconToButton(pbMaximize, unmaximize_png, unmaximize_png_len, TQString("Restore"));
	}
}

void Calculator::slotCloseClicked()
{
	close();
}

void Calculator::slotHistoryClicked()
{
	if (!m_historyWindow) {
		m_historyWindow = new HistoryWindow(this);
		connect(m_historyWindow, TQ_SIGNAL(itemSelected(const CalcHistoryItem&)), this, TQ_SLOT(slotHistoryItemSelected(const CalcHistoryItem&)));
		connect(m_historyWindow, TQ_SIGNAL(clearHistoryRequested()), this, TQ_SLOT(slotClearHistory()));
		m_historyWindow->setColors(m_colorPanelBg, m_colorDisplayFg, m_colorDisplayBg);
	}

	if (m_historyWindow->isVisible()) {
		m_historyWindow->hide();
	} else {
		if (_calc_mode <= ModeStatistics) {
			m_historyWindow->setHistory(m_history[_calc_mode]);
		} else {
			std::vector<CalcHistoryItem> empty_hist;
			m_historyWindow->setHistory(empty_hist);
		}
		m_historyWindow->retranslateUi();
		m_historyWindow->show();
		m_historyWindow->raise();
		m_historyWindow->setActiveWindow();
	}
}

void Calculator::slotHistoryItemSelected(const CalcHistoryItem &item)
{
	_expr_text = item.expression;
	_expr_unary = TQString();
	_expr_just_equaled = true;
	_expr_op_pending = false;

	calc_display->sendEvent(CalcDisplay::EventReset);
	calc_display->setAmount(item.result_num);
	UpdateDisplay(false);
}

void Calculator::slotClearHistory()
{
	if (_calc_mode <= ModeStatistics) {
		m_history[_calc_mode].clear();
		if (m_historyWindow) {
			m_historyWindow->setHistory(m_history[_calc_mode]);
		}
	}
}

void Calculator::slotConstantToDisplay(int constant)
{
	if (mConverterPage && mConverterPage->isVisible()) {
		double val_double = CalcConstMenu::Constants[constant].value.toDouble();
		TQString targetStr = formatDouble(val_double);
		if (m_convActiveSlot == 1) {
			m_convInput1 = targetStr;
			setVal1Text(targetStr);
		} else {
			m_convInput2 = targetStr;
			setVal2Text(targetStr);
		}
		recalculateConversion();
		return;
	}

	calc_display->setAmount(CalcConstMenu::Constants[constant].value);

	UpdateDisplay(false);
}

void Calculator::updateGeometry(void)
{
    TQObjectList l;
    TQSize s;
    int margin;

    //
    // Calculator buttons
    //
    s.setWidth(mSmallPage->fontMetrics().width("MMMM"));
    s.setHeight(mSmallPage->fontMetrics().lineSpacing());

    // why this stupid cast!
    l = mSmallPage->childrenListObject();

    for(uint i=0; i < l.count(); i++)
    {
        TQObject *o = l.at(i);
        if( o->isWidgetType() )
        {
            TQWidget *tmp_widget = dynamic_cast<TQWidget *>(o);
            margin = TQApplication::style().
                pixelMetric(TQStyle::PM_ButtonMargin, (tmp_widget))*2;
            tmp_widget->setFixedSize(s.width()+margin, s.height()+margin);
            //tmp_widget->setMinimumSize(s.width()+margin, s.height()+margin);
            tmp_widget->installEventFilter( this );
            tmp_widget->setAcceptDrops(true);
        }
    }

    l = mLargePage->childrenListObject();

    int h1 = (NumButtonGroup->find(0x0F))->minimumSize().height();
    int h2 = static_cast<int>( (static_cast<float>(h1) + 4.0) / 5.0 );
    s.setWidth(mLargePage->fontMetrics().width("MMM") +
               TQApplication::style().
               pixelMetric(TQStyle::PM_ButtonMargin, NumButtonGroup->find(0x0F))*2);
    s.setHeight(h1 + h2);

    for(uint i = 0; i < l.count(); i++)
    {
        TQObject *o = l.at(i);
        if(o->isWidgetType())
        {
            TQWidget *tmp_widget = dynamic_cast<TQWidget *>(o);
            tmp_widget->setFixedSize(s);
            tmp_widget->installEventFilter(this);
            tmp_widget->setAcceptDrops(true);
        }
    }

    pbInv->setFixedSize(s);
    pbInv->installEventFilter(this);
    pbInv->setAcceptDrops(true);



    l = mNumericPage->childrenListObject(); // silence please

    h1 = (NumButtonGroup->find(0x0F))->minimumSize().height();
    h2 = (int)((((float)h1 + 4.0) / 5.0));
    s.setWidth(mLargePage->fontMetrics().width("MMM") +
               TQApplication::style().
               pixelMetric(TQStyle::PM_ButtonMargin, NumButtonGroup->find(0x0F))*2);
    s.setHeight(h1 + h2);

    for(uint i = 0; i < l.count(); i++)
    {
        TQObject *o = l.at(i);
        if(o->isWidgetType())
        {
            TQWidget *tmp_widget = dynamic_cast<TQWidget *>(o);
            tmp_widget->setFixedSize(s);
            tmp_widget->installEventFilter(this);
            tmp_widget->setAcceptDrops(true);
        }
    }

    // Set Buttons of double size
    TQSize t(s);
    t.setWidth(2*s.width());
    NumButtonGroup->find(0x00)->setFixedSize(t);
    t = s;
    t.setHeight(2*s.height());
    pbEqual->setFixedSize(t);
    pbPlus->setFixedSize(t);
}

void Calculator::slotBaseSelected(int base)
{
	int current_base;

	// set display & statusbar (if item exist in statusbar)
	switch(base)
	{
	case 3:
	  current_base = calc_display->setBase(NumBase(2));
	  //if (statusBar()->hasItem(1)) statusBar()->changeItem("BIN",1);
	  calc_display->setStatusText(1, "Bin");
	  break;
	case 2:
	  current_base = calc_display->setBase(NumBase(8));
	  //if (statusBar()->hasItem(1)) statusBar()->changeItem("OCT",1);
	  calc_display->setStatusText(1, "Oct");
	  break;
	case 1:
	  current_base = calc_display->setBase(NumBase(10));
	  //if (statusBar()->hasItem(1)) statusBar()->changeItem("DEC",1);
	  calc_display->setStatusText(1, "Dec");
	  break;
	case 0:
	  current_base = calc_display->setBase(NumBase(16));
	  //if (statusBar()->hasItem(1)) statusBar()->changeItem("HEX",1);
	  calc_display->setStatusText(1, "Hex");
	  break;
	default:
	  //if (statusBar()->hasItem(1)) statusBar()->changeItem("Error",1);
	  calc_display->setStatusText(1, "Error");
	  return;
	}

	// Enable the buttons not available in this base
	for (int i=0; i<current_base; i++) {
	  if (NumButtonGroup->find(i)) NumButtonGroup->find(i)->setEnabled (true);
	  if (ProgNumButtonGroup && ProgNumButtonGroup->find(i)) ProgNumButtonGroup->find(i)->setEnabled(true);
	}

	// Disable the buttons not available in this base
	for (int i=current_base; i<16; i++) {
	  if (NumButtonGroup->find(i)) NumButtonGroup->find(i)->setEnabled (false);
	  if (ProgNumButtonGroup && ProgNumButtonGroup->find(i)) ProgNumButtonGroup->find(i)->setEnabled(false);
	}

	// Only enable the decimal point in decimal
	pbPeriod->setEnabled(current_base == NB_DECIMAL);
	// Only enable the x*10^y button in decimal
	pbEE->setEnabled(current_base == NB_DECIMAL);

	// Disable buttons that make only sense with floating point
	// numbers
	if(current_base != NB_DECIMAL)
	{
	  pbScientific["HypMode"]->setEnabled(false);
	  pbScientific["Sine"]->setEnabled(false);
	  pbScientific["Cosine"]->setEnabled(false);
	  pbScientific["Tangent"]->setEnabled(false);
	  pbScientific["LogNatural"]->setEnabled(false);
	  pbScientific["Log10"]->setEnabled(false);
	}
	else
	{
	  pbScientific["HypMode"]->setEnabled(true);
	  pbScientific["Sine"]->setEnabled(true);
	  pbScientific["Cosine"]->setEnabled(true);
	  pbScientific["Tangent"]->setEnabled(true);
	  pbScientific["LogNatural"]->setEnabled(true);
	  pbScientific["Log10"]->setEnabled(true);
	}
}

void Calculator::keyPressEvent(TQKeyEvent *e)
{
	int keyVal = e->key();
	if (keyVal == Key_Escape) {
		close();
		return;
	}

	if (_calc_mode != ModeVolume) {
		if (e->text() == "(" || keyVal == Key_ParenLeft) {
			if (_calc_mode == ModeScientific) {
				if (pbSciParenL && pbSciParenL->isEnabled()) {
					pbSciParenL->animateClick();
					return;
				}
			} else if (_calc_mode == ModeProgrammer) {
				if (pbProgParenL && pbProgParenL->isEnabled()) {
					pbProgParenL->animateClick();
					return;
				}
			} else if (_calc_mode != ModeStandard) {
				if (pbParenOpen && pbParenOpen->isEnabled()) {
					pbParenOpen->animateClick();
					return;
				}
			}
		} else if (e->text() == ")" || keyVal == Key_ParenRight) {
			if (_calc_mode == ModeScientific) {
				if (pbSciParenR && pbSciParenR->isEnabled()) {
					pbSciParenR->animateClick();
					return;
				}
			} else if (_calc_mode == ModeProgrammer) {
				if (pbProgParenR && pbProgParenR->isEnabled()) {
					pbProgParenR->animateClick();
					return;
				}
			} else if (_calc_mode != ModeStandard) {
				if (pbParenClose && pbParenClose->isEnabled()) {
					pbParenClose->animateClick();
					return;
				}
			}
		} else if (e->text() == "." || e->text() == "," || keyVal == Key_Period || keyVal == Key_Comma) {
			if (_calc_mode == ModeStandard) {
				if (pbStdPeriod && pbStdPeriod->isEnabled()) {
					pbStdPeriod->animateClick();
					return;
				}
			} else if (_calc_mode == ModeScientific) {
				if (pbSciDot && pbSciDot->isEnabled()) {
					pbSciDot->animateClick();
					return;
				}
			} else {
				if (pbPeriod && pbPeriod->isEnabled()) {
					pbPeriod->animateClick();
					return;
				}
			}
		}
	}

	if (keyVal == Key_Enter || keyVal == Key_Return || keyVal == Key_Equal) {
		if (_calc_mode == ModeStandard) {
			if (pbStdEqual && pbStdEqual->isEnabled()) {
				pbStdEqual->animateClick();
				return;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciEq && pbSciEq->isEnabled()) {
				pbSciEq->animateClick();
				return;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgEq && pbProgEq->isEnabled()) {
				pbProgEq->animateClick();
				return;
			}
		} else if (_calc_mode == ModeStatistics) {
			if (pbEqual && pbEqual->isEnabled()) {
				pbEqual->animateClick();
				return;
			}
		}
	}

	if (keyVal == Key_Delete) {
		if (_calc_mode == ModeStandard) {
			if (pbStdCE && pbStdCE->isEnabled()) {
				pbStdCE->animateClick();
				return;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciCE && pbSciCE->isEnabled()) {
				pbSciCE->animateClick();
				return;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgCE && pbProgCE->isEnabled()) {
				pbProgCE->animateClick();
				return;
			}
		} else if (_calc_mode == ModeStatistics) {
			if (pbClear && pbClear->isEnabled()) {
				pbClear->animateClick();
				return;
			}
		}
	}

	if (e->text() == TQString::fromUtf8("²") || keyVal == Key_twosuperior) {
		if (pbSquare && pbSquare->isEnabled()) {
			pbSquare->animateClick();
			return;
		}
	}

	if (e->text() == "%" || keyVal == Key_Percent) {
		if (_calc_mode == ModeStandard) {
			if (pbStdPercent && pbStdPercent->isEnabled()) {
				pbStdPercent->animateClick();
				return;
			}
		} else if (_calc_mode == ModeScientific || _calc_mode == ModeProgrammer || _calc_mode == ModeStatistics) {
			if (pbPercent && pbPercent->isEnabled()) {
				pbPercent->animateClick();
				return;
			}
		}
	}

	if (_calc_mode >= ModeVolume) {
		int k = e->key();
		if (_calc_mode == ModeRoman && m_convActiveSlot == 2) {
			TQString text = e->text().upper();
			if (text == "I") { handleConvDigit(1); return; }
			if (text == "V") { handleConvDigit(2); return; }
			if (text == "X") { handleConvDigit(3); return; }
			if (text == "L") { handleConvDigit(4); return; }
			if (text == "C") { handleConvDigit(5); return; }
			if (text == "D") { handleConvDigit(6); return; }
			if (text == "M") { handleConvDigit(7); return; }

			if (k >= Key_1 && k <= Key_7) {
				handleConvDigit(k - Key_0);
				return;
			}
			if (k == Key_Backspace) {
				handleConvBS();
				return;
			}
			if (k == Key_Delete) {
				handleConvCE();
				return;
			}
			if (k == Key_Tab) {
				setActiveSlot((m_convActiveSlot == 1) ? 2 : 1);
				return;
			}
			return;
		}

		if (k >= Key_0 && k <= Key_9) {
			handleConvDigit(k - Key_0);
			return;
		}
		if (k == Key_Period || k == Key_Comma) {
			if (_calc_mode != ModeRoman) {
				handleConvComma();
			}
			return;
		}
		if (k == Key_Backspace) {
			handleConvBS();
			return;
		}
		if (k == Key_Delete) {
			handleConvCE();
			return;
		}
		if (k == Key_Tab) {
			setActiveSlot((m_convActiveSlot == 1) ? 2 : 1);
			return;
		}
	}

    if ( ( e->state() & KeyButtonMask ) == 0 || ( e->state() & ShiftButton ) ) {
	switch (e->key())
	{
	case Key_Next:
		pbAC->animateClick();
		break;
	case Key_Slash:
	case Key_division:
		if (_calc_mode == ModeStandard) {
			if (pbStdDivision && pbStdDivision->isEnabled()) pbStdDivision->animateClick();
		} else if (_calc_mode == ModeScientific) {
			if (pbSciDiv && pbSciDiv->isEnabled()) pbSciDiv->animateClick();
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgDiv && pbProgDiv->isEnabled()) pbProgDiv->animateClick();
		} else {
			if (pbDivision && pbDivision->isEnabled()) pbDivision->animateClick();
		}
		break;
	case Key_Asterisk:
	case Key_multiply:
		if (_calc_mode == ModeStandard) {
			if (pbStdX && pbStdX->isEnabled()) pbStdX->animateClick();
		} else if (_calc_mode == ModeScientific) {
			if (pbSciMul && pbSciMul->isEnabled()) pbSciMul->animateClick();
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgMul && pbProgMul->isEnabled()) pbProgMul->animateClick();
		} else {
			if (pbX && pbX->isEnabled()) pbX->animateClick();
		}
		break;
	case Key_Plus:
		if (_calc_mode == ModeStandard) {
			if (pbStdPlus && pbStdPlus->isEnabled()) pbStdPlus->animateClick();
		} else if (_calc_mode == ModeScientific) {
			if (pbSciAdd && pbSciAdd->isEnabled()) pbSciAdd->animateClick();
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgAdd && pbProgAdd->isEnabled()) pbProgAdd->animateClick();
		} else {
			if (pbPlus && pbPlus->isEnabled()) pbPlus->animateClick();
		}
		break;
	case Key_Minus:
		if (_calc_mode == ModeStandard) {
			if (pbStdMinus && pbStdMinus->isEnabled()) pbStdMinus->animateClick();
		} else if (_calc_mode == ModeScientific) {
			if (pbSciSub && pbSciSub->isEnabled()) pbSciSub->animateClick();
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgSub && pbProgSub->isEnabled()) pbProgSub->animateClick();
		} else {
			if (pbMinus && pbMinus->isEnabled()) pbMinus->animateClick();
		}
		break;
 	case Key_D:
		pbStat["InputData"]->animateClick(); // stat mode
		break;
	case Key_BracketLeft:
        case Key_twosuperior:
		pbSquare->animateClick();
		break;
	case Key_Prior:
		if (_calc_mode == ModeStandard) {
			if (pbStdC && pbStdC->isEnabled()) pbStdC->animateClick();
		} else if (_calc_mode == ModeScientific) {
			if (pbSciC && pbSciC->isEnabled()) pbSciC->animateClick();
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgC && pbProgC->isEnabled()) pbProgC->animateClick();
		} else {
			if (pbClear && pbClear->isEnabled()) pbClear->animateClick();
		}
		break;
	case Key_Backspace:
		if (_calc_mode == ModeStandard) {
			if (pbStdBackSpace && pbStdBackSpace->isEnabled()) {
				pbStdBackSpace->animateClick();
				return;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciBS && pbSciBS->isEnabled()) {
				pbSciBS->animateClick();
				return;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgBS && pbProgBS->isEnabled()) {
				pbProgBS->animateClick();
				return;
			}
		}
		calc_display->deleteLastDigit();
		break;
	}
    }

    // Do not show accelerators on Ctrl press
    /*
    if (e->key() == Key_Control)
	emit switchShowAccels(true);
    */
}

void Calculator::keyReleaseEvent(TQKeyEvent *e)
{
    // Do not show accelerators on Ctrl release
    /*
    if (e->key() == Key_Control)
	emit switchShowAccels(false);
    */
}

void Calculator::slotAngleSelected(int number)
{
	if (pbAngleChoose && pbAngleChoose->popup()) {
		pbAngleChoose->popup()->setItemChecked(0, false);
		pbAngleChoose->popup()->setItemChecked(1, false);
		pbAngleChoose->popup()->setItemChecked(2, false);
	}

	switch(number)
	{
	case 0:
		_angle_mode = DegMode;
		//statusBar()->changeItem("DEG", 2);
		if (pbAngleChoose && pbAngleChoose->popup()) pbAngleChoose->popup()->setItemChecked(0, true);
		calc_display->setStatusText(2, "Deg");
		if (pbSciAngle) pbSciAngle->setText("DEG");
		break;
	case 1:
		_angle_mode = RadMode;
		//statusBar()->changeItem("RAD", 2);
		if (pbAngleChoose && pbAngleChoose->popup()) pbAngleChoose->popup()->setItemChecked(1, true);
		calc_display->setStatusText(2, "Rad");
		if (pbSciAngle) pbSciAngle->setText("RAD");
		break;
	case 2:
		_angle_mode = GradMode;
		//statusBar()->changeItem("GRA", 2);
		if (pbAngleChoose && pbAngleChoose->popup()) pbAngleChoose->popup()->setItemChecked(2, true);
		calc_display->setStatusText(2, "Gra");
		if (pbSciAngle) pbSciAngle->setText("GRAD");
		break;
	default: // we shouldn't ever end up here
		_angle_mode = RadMode;
	}
}

void Calculator::slotEEclicked(void)
{
	calc_display->newCharacter('e');
}

void Calculator::slotInvtoggled(bool flag)
{
	inverse = flag;

	if (pbSciInv && pbSciInv->isOn() != flag) {
		pbSciInv->blockSignals(true);
		pbSciInv->setOn(flag);
		pbSciInv->blockSignals(false);
	}

	emit switchMode(ModeInverse, flag);

	if (inverse)
	{
		//statusBar()->changeItem("INV", 0);
		calc_display->setStatusText(0, "Inv");
	}
	else
	{
		//statusBar()->changeItem("NORM", 0);
		calc_display->setStatusText(0, TQString());
	}
}

void Calculator::slotHyptoggled(bool flag)
{
	// toggle between hyperbolic and standart trig functions
	hyp_mode = flag;

	emit switchMode(ModeHyperbolic, flag);
}



void Calculator::slotMemRecallclicked(void)
{
	// temp. work-around
	calc_display->sendEvent(CalcDisplay::EventReset);

	calc_display->setAmount(memory_num);
	UpdateDisplay(false);
}

void Calculator::recallMemoryEntry(const KNumber &val)
{
	calc_display->sendEvent(CalcDisplay::EventReset);
	calc_display->setAmount(val);
	UpdateDisplay(false);
}

void Calculator::slotMemStoreclicked(void)
{
	EnterEqual();

	memory_num = calc_display->getAmount();
	calc_display->setStatusText(3, "M");
	//statusBar()->changeItem("M",3);
	pbMemRecall->setEnabled(true);
}

void Calculator::slotNumberclicked(int number_clicked)
{
	if (_expr_just_equaled) {
		// New number after '=': reset expression
		_expr_text = TQString();
		_expr_just_equaled = false;
		_expr_unary = TQString();
	}
	_expr_op_pending = false;
	calc_display->EnterDigit(number_clicked);
}

void Calculator::slotSinclicked(void)
{
	if (hyp_mode)
	{
		// sinh or arsinh
		if (!inverse) {
			exprUnaryOp("sinh");
			core.SinHyp(calc_display->getAmount());
		} else {
			exprUnaryOp("asinh");
			core.AreaSinHyp(calc_display->getAmount());
		}
	}
	else
	{
		// sine or arcsine
		if (!inverse) {
			exprUnaryOp("sin");
			switch(_angle_mode)
			{
			case DegMode:
				core.SinDeg(calc_display->getAmount());
				break;
			case RadMode:
				core.SinRad(calc_display->getAmount());
				break;
			case GradMode:
				core.SinGrad(calc_display->getAmount());
				break;
			}
		} else {
			exprUnaryOp("asin");
			switch(_angle_mode)
			{
			case DegMode:
				core.ArcSinDeg(calc_display->getAmount());
				break;
			case RadMode:
				core.ArcSinRad(calc_display->getAmount());
				break;
			case GradMode:
				core.ArcSinGrad(calc_display->getAmount());
				break;
			}
		}
	}

	UpdateDisplay(true);
}

void Calculator::slotPlusMinusclicked(void)
{
	// display can only change sign, when in input mode, otherwise we
	// need the core to do this.
	if (!calc_display->sendEvent(CalcDisplay::EventChangeSign))
	{
	    core.InvertSign(calc_display->getAmount());
	    UpdateDisplay(true);
	}
}

void Calculator::slotMemPlusMinusclicked(void)
{
	bool tmp_inverse = inverse; // store this, because next command deletes inverse
	EnterEqual(); // finish calculation so far, to store result into MEM

	if (!tmp_inverse)	memory_num += calc_display->getAmount();
	else 			memory_num -= calc_display->getAmount();

	pbInv->setOn(false);
	//statusBar()->changeItem("M",3);
	calc_display->setStatusText(3, "M");
	pbMemRecall->setEnabled(true);
}

void Calculator::slotStdMPlusclicked(void)
{
	EnterEqual();
	memory_num += calc_display->getAmount();
	//statusBar()->changeItem("M",3);
	calc_display->setStatusText(3, "M");
	pbMemRecall->setEnabled(true);
	if (pbStdMC) pbStdMC->setEnabled(true);
	if (pbStdMR) pbStdMR->setEnabled(true);
}

void Calculator::slotStdMMinusclicked(void)
{
	EnterEqual();
	memory_num -= calc_display->getAmount();
	//statusBar()->changeItem("M",3);
	calc_display->setStatusText(3, "M");
	pbMemRecall->setEnabled(true);
	if (pbStdMC) pbStdMC->setEnabled(true);
	if (pbStdMR) pbStdMR->setEnabled(true);
}

void Calculator::slotHamburgerClicked(void)
{
	TQColor menuBg = m_colorMenuBg;
	TQColor panelFg = m_colorPanelFg;
	int val = (menuBg.red() * 299 + menuBg.green() * 587 + menuBg.blue() * 114) / 1000;
	TQColor highlightColor = (val > 128) ? menuBg.dark(112) : menuBg.light(125);
	if (val < 10) {
		highlightColor = TQColor(50, 50, 50);
	}
	TQColor highlightText = (val > 128) ? TQColor(0, 0, 0) : TQColor(255, 255, 255);

	TQPopupMenu *menu = new TQPopupMenu(this);
	TQPalette pal = menu->palette();
	pal.setColor(TQPalette::Active, TQColorGroup::Background, menuBg);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Background, menuBg);
	pal.setColor(TQPalette::Active, TQColorGroup::Button, menuBg);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Button, menuBg);
	pal.setColor(TQPalette::Active, TQColorGroup::Foreground, panelFg);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, panelFg);
	pal.setColor(TQPalette::Active, TQColorGroup::ButtonText, panelFg);
	pal.setColor(TQPalette::Inactive, TQColorGroup::ButtonText, panelFg);
	pal.setColor(TQPalette::Active, TQColorGroup::Highlight, highlightColor);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Highlight, highlightColor);
	pal.setColor(TQPalette::Active, TQColorGroup::HighlightedText, highlightText);
	pal.setColor(TQPalette::Inactive, TQColorGroup::HighlightedText, highlightText);
	menu->setPalette(pal);

	bool is_scientific = (_calc_mode == ModeScientific);
	bool is_logic = (_calc_mode == ModeProgrammer);
	bool is_stat = (_calc_mode == ModeStatistics);
	bool is_standard = (_calc_mode == ModeStandard);

	menu->insertItem(new CalcMenuItem(standard_png, standard_png_len, tr_str("Standard"), is_standard), 100);
	menu->insertItem(new CalcMenuItem(science_png, science_png_len, tr_str("Scientific"), is_scientific), 101);
	menu->insertItem(new CalcMenuItem(coder_png, coder_png_len, tr_str("Programmer"), is_logic), 102);
	menu->insertItem(new CalcMenuItem(date_png, date_png_len, tr_str("Date calculation"), (_calc_mode == ModeDateCalc)), 103);
	
	menu->insertSeparator();
	
	menu->insertItem(new CalcMenuItem(volume_png, volume_png_len, tr_str("Volume"), (_calc_mode == ModeVolume)), 200);
	menu->insertItem(new CalcMenuItem(lenght_png, lenght_png_len, tr_str("Length"), (_calc_mode == ModeLength)), 201);
	menu->insertItem(new CalcMenuItem(mass_png, mass_png_len, tr_str("Weight and mass"), (_calc_mode == ModeMass)), 202);
	menu->insertItem(new CalcMenuItem(temp_png, temp_png_len, tr_str("Temperature"), (_calc_mode == ModeTemp)), 203);
	menu->insertItem(new CalcMenuItem(energy_png, energy_png_len, tr_str("Energy"), (_calc_mode == ModeEnergy)), 204);
	menu->insertItem(new CalcMenuItem(area_png, area_png_len, tr_str("Area"), (_calc_mode == ModeArea)), 205);
	menu->insertItem(new CalcMenuItem(speed_png, speed_png_len, tr_str("Speed"), (_calc_mode == ModeSpeed)), 206);
	menu->insertItem(new CalcMenuItem(time_png, time_png_len, tr_str("Time"), (_calc_mode == ModeTime)), 207);
	menu->insertItem(new CalcMenuItem(power_png, power_png_len, tr_str("Power"), (_calc_mode == ModePower)), 208);
	menu->insertItem(new CalcMenuItem(data_png, data_png_len, tr_str("Data"), (_calc_mode == ModeData)), 209);
	menu->insertItem(new CalcMenuItem(pressure_png, pressure_png_len, tr_str("Pressure"), (_calc_mode == ModePressure)), 210);
	menu->insertItem(new CalcMenuItem(angle_png, angle_png_len, tr_str("Angle"), (_calc_mode == ModeAngle)), 211);
	menu->insertItem(new CalcMenuItem(roman_png, roman_png_len, tr_str("Roman numerals"), (_calc_mode == ModeRoman)), 212);

	menu->insertSeparator();
	menu->insertItem(new CalcMenuItem(settings_png, settings_png_len, tr_str("Settings"), false), 300);

	menu->setMinimumWidth(190);
	TQPoint pos = this->mapToGlobal(TQPoint(0, 0));
	pos.setY(pos.y() + 1);
	int selected = menu->exec(pos);

	if (selected == 100) {
		_calc_mode = ModeStandard;
		updateModeVisibility();
	} else if (selected == 101) {
		_calc_mode = ModeScientific;
		calc_display->setStatusText(2, "Deg");
		slotAngleSelected(0);
		updateModeVisibility();
	} else if (selected == 102) {
		_calc_mode = ModeProgrammer;
		calc_display->setStatusText(1, "Hex");
		resetBase();
		BaseChooseGroup->show();
		for (int i=10; i<16; i++)
			(NumButtonGroup->find(i))->show();
		updateModeVisibility();
	} else if (selected == 103) {
		_calc_mode = ModeDateCalc;
		updateModeVisibility();
	} else if (selected >= 200 && selected <= 212) {
		_calc_mode = static_cast<CalcMode>(ModeVolume + (selected - 200));
		updateModeVisibility();
	} else if (selected == 300) {
		showSettings();
	}

	delete menu;
}

// slots removed

void Calculator::slotBackSpaceclicked(void)
{
	calc_display->deleteLastDigit();
}

void Calculator::slotCosclicked(void)
{
	if (hyp_mode)
	{
		// cosh or arcosh
		if (!inverse) {
			exprUnaryOp("cosh");
			core.CosHyp(calc_display->getAmount());
		} else {
			exprUnaryOp("acosh");
			core.AreaCosHyp(calc_display->getAmount());
		}
	}
	else
	{
		// cosine or arccosine
		if (!inverse) {
			exprUnaryOp("cos");
			switch(_angle_mode)
			{
			case DegMode:
				core.CosDeg(calc_display->getAmount());
				break;
			case RadMode:
				core.CosRad(calc_display->getAmount());
				break;
			case GradMode:
				core.CosGrad(calc_display->getAmount());
				break;
			}
		} else {
			exprUnaryOp("acos");
			switch(_angle_mode)
			{
			case DegMode:
				core.ArcCosDeg(calc_display->getAmount());
				break;
			case RadMode:
				core.ArcCosRad(calc_display->getAmount());
				break;
			case GradMode:
				core.ArcCosGrad(calc_display->getAmount());
				break;
			}
		}
	}

	UpdateDisplay(true);
}

void Calculator::slotReciclicked(void)
{
	exprUnaryOp("1/");
	core.Reciprocal(calc_display->getAmount());
	UpdateDisplay(true);
}

void Calculator::slotTanclicked(void)
{
	if (hyp_mode)
	{
		// tanh or artanh
		if (!inverse) {
			exprUnaryOp("tanh");
			core.TangensHyp(calc_display->getAmount());
		} else {
			exprUnaryOp("atanh");
			core.AreaTangensHyp(calc_display->getAmount());
		}
	}
	else
	{
		// tan or arctan
		if (!inverse) {
			exprUnaryOp("tan");
			switch(_angle_mode)
			{
			case DegMode:
				core.TangensDeg(calc_display->getAmount());
				break;
			case RadMode:
				core.TangensRad(calc_display->getAmount());
				break;
			case GradMode:
				core.TangensGrad(calc_display->getAmount());
				break;
			}
		} else {
			exprUnaryOp("atan");
			switch(_angle_mode)
			{
			case DegMode:
				core.ArcTangensDeg(calc_display->getAmount());
				break;
			case RadMode:
				core.ArcTangensRad(calc_display->getAmount());
				break;
			case GradMode:
				core.ArcTangensGrad(calc_display->getAmount());
				break;
			}
		}
	}

	UpdateDisplay(true);
}

void Calculator::slotFactorialclicked(void)
{
	exprUnaryOp("fact");
	core.Factorial(calc_display->getAmount());

	UpdateDisplay(true);
}

void Calculator::slotLogclicked(void)
{
	if (!inverse) {
		exprUnaryOp("log");
		core.Log10(calc_display->getAmount());
	} else {
		exprUnaryOp("10^"); // 10^x
		core.Exp10(calc_display->getAmount());
	}

	UpdateDisplay(true);
}

void Calculator::slot10xclicked(void)
{
	exprUnaryOp("10^"); // 10^x
	core.Exp10(calc_display->getAmount());
	UpdateDisplay(true);
}

void Calculator::slotPiclicked(void)
{
	calc_display->setAmount(KNumber::Pi);
	UpdateDisplay(false);
}


void Calculator::slotSquareclicked(void)
{
	if (!inverse)
	{
		exprUnaryOp("sqr");
		core.Square(calc_display->getAmount());
	}
	else
	{
		exprUnaryOp("cube");
		core.Cube(calc_display->getAmount());
	}

	UpdateDisplay(true);
}

void Calculator::slotRootclicked(void)
{
	if (!inverse)
	{
		exprUnaryOp(TQString::fromUtf8("\xe2\x88\x9a"));
		core.SquareRoot(calc_display->getAmount());
	}
	else
	{
		exprUnaryOp(TQString::fromUtf8("\xc2\xb3\xe2\x88\x9a"));
		core.CubeRoot(calc_display->getAmount());
	}

	UpdateDisplay(true);
}

void Calculator::slotLnclicked(void)
{
	if (!inverse) {
		exprUnaryOp("ln");
		core.Ln(calc_display->getAmount());
	} else {
		exprUnaryOp("exp");
		core.Exp(calc_display->getAmount());
	}

	UpdateDisplay(true);
}

void Calculator::slotPowerclicked(void)
{
	if (inverse)
	{
		exprBinaryOp(TQString::fromUtf8("\xca\xb8\xe2\x88\x9a")); // y-th root
		core.enterOperation(calc_display->getAmount(),
				    CalcEngine::FUNC_PWR_ROOT);
		pbInv->setOn(false);
	}
	else
	{
		exprBinaryOp("^");
		core.enterOperation(calc_display->getAmount(),
				    CalcEngine::FUNC_POWER);
	}
	// temp. work-around
	KNumber tmp_num = calc_display->getAmount();
	calc_display->sendEvent(CalcDisplay::EventReset);
	calc_display->setAmount(tmp_num);
	UpdateDisplay(false);
}

void Calculator::slotMCclicked(void)
{
	memory_num		= 0;
	//statusBar()->changeItem("    ",3);
	calc_display->setStatusText(3, TQString());
	pbMemRecall->setEnabled(false);
	if (pbStdMC) pbStdMC->setEnabled(false);
	if (pbStdMR) pbStdMR->setEnabled(false);
}

void Calculator::slotClearclicked(void)
{
	// CE: clear entry only, expression stays
	calc_display->sendEvent(CalcDisplay::EventClear);
}

void Calculator::slotACclicked(void)
{
	core.Reset();
	calc_display->sendEvent(CalcDisplay::EventReset);

	// AC: clear expression
	_expr_text = TQString();
	_expr_unary = TQString();
	_expr_just_equaled = false;
	_expr_op_pending = false;

	UpdateDisplay(true);
}

void Calculator::slotParenOpenclicked(void)
{
	_expr_text += "( ";
	_expr_op_pending = false;
	_expr_just_equaled = false;
	core.ParenOpen(calc_display->getAmount());
	UpdateDisplay(false);
}

void Calculator::slotParenCloseclicked(void)
{
	// Append current operand then close paren
	TQString operand_str;
	if (!_expr_unary.isEmpty()) {
		operand_str = _expr_unary;
		_expr_unary = TQString();
	} else {
		operand_str = exprFormatNumber(calc_display->getAmount());
	}
	if (_expr_op_pending) {
		_expr_text += operand_str;
		_expr_op_pending = false;
	} else if (!_expr_text.stripWhiteSpace().endsWith(")")) {
		_expr_text += operand_str;
	}
	_expr_text += " ) ";
	core.ParenClose(calc_display->getAmount());

	UpdateDisplay(true);
}

void Calculator::slotANDclicked(void)
{
	exprBinaryOp("AND");
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_AND);

	UpdateDisplay(true);
}

void Calculator::slotXclicked(void)
{
	exprBinaryOp(TQString::fromUtf8("\xc3\x97"));
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_MULTIPLY);

	UpdateDisplay(true);
}

void Calculator::slotDivisionclicked(void)
{
	exprBinaryOp(TQString::fromUtf8("\xc3\xb7"));
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_DIVIDE);

	UpdateDisplay(true);
}

void Calculator::slotORclicked(void)
{
	exprBinaryOp("OR");
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_OR);

	UpdateDisplay(true);
}

void Calculator::slotXORclicked(void)
{
	exprBinaryOp("XOR");
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_XOR);

	UpdateDisplay(true);
}

void Calculator::slotPlusclicked(void)
{
	exprBinaryOp("+");
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_ADD);

	UpdateDisplay(true);
}

void Calculator::slotMinusclicked(void)
{
	exprBinaryOp("-");
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_SUBTRACT);

	UpdateDisplay(true);
}

void Calculator::slotLeftShiftclicked(void)
{
	exprBinaryOp("Lsh");
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_LSH);

	UpdateDisplay(true);
}

void Calculator::slotRightShiftclicked(void)
{
	exprBinaryOp("Rsh");
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_RSH);

	UpdateDisplay(true);
}

void Calculator::slotPeriodclicked(void)
{
	if (_expr_just_equaled) {
		_expr_text = TQString();
		_expr_just_equaled = false;
		_expr_unary = TQString();
	}
	_expr_op_pending = false;
	calc_display->newCharacter('.');
}

void Calculator::EnterEqual()
{
	// Finalize expression: append operand and '='
	if (!_expr_text.isEmpty()) {
		TQString operand_str;
		if (!_expr_unary.isEmpty()) {
			operand_str = _expr_unary;
			_expr_unary = TQString();
		} else {
			operand_str = exprFormatNumber(calc_display->getAmount());
		}
		if (_expr_op_pending) {
			// Operator with no second operand typed (e.g. "5 + =")
			// Windows calc duplicates: "5 + 5 ="
			_expr_text += operand_str + " =";
		} else {
			// Normal: user typed a second operand
			_expr_text += operand_str + " =";
		}
	}

	_expr_just_equaled = true;
	_expr_op_pending = false;

	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_EQUAL);

	UpdateDisplay(true, true);

	if (!_expr_text.isEmpty() && _calc_mode <= ModeStatistics) {
		CalcHistoryItem item;
		item.expression = _expr_text;
		item.result_str = calc_display->text();
		item.result_num = calc_display->getAmount();

		std::vector<CalcHistoryItem> &hist = m_history[_calc_mode];
		if (hist.size() >= 20) {
			hist.erase(hist.begin());
		}
		hist.push_back(item);

		if (m_historyWindow && m_historyWindow->isVisible()) {
			m_historyWindow->setHistory(hist);
		}
	}
}

void Calculator::slotEqualclicked(void)
{
	EnterEqual();
}

void Calculator::slotPercentclicked(void)
{
	_expr_just_equaled = false;
	TQString operand_str;
	if (!_expr_unary.isEmpty()) {
		operand_str = _expr_unary;
	} else {
		operand_str = exprFormatNumber(calc_display->getAmount());
	}
	_expr_unary = operand_str + "%";

	// Percent doesn't wrap in expression; the result replaces the operand directly
	core.enterOperation(calc_display->getAmount(),
			    CalcEngine::FUNC_PERCENT);

	UpdateDisplay(true);
}

void Calculator::slotNegateclicked(void)
{
	exprUnaryOp("Not");
	core.Complement(calc_display->getAmount());

	UpdateDisplay(true);
}

void Calculator::slotModclicked(void)
{
	if (inverse) {
		exprBinaryOp("IntDiv");
		core.enterOperation(calc_display->getAmount(),
				    CalcEngine::FUNC_INTDIV);
	} else {
		exprBinaryOp("Mod");
		core.enterOperation(calc_display->getAmount(),
				    CalcEngine::FUNC_MOD);
	}

	UpdateDisplay(true);
}

void Calculator::slotStatNumclicked(void)
{
	if(!inverse)
	{
		core.StatCount(0);
	}
	else
	{
		pbInv->setOn(false);
		core.StatSum(0);
	}

	UpdateDisplay(true);
}

void Calculator::slotStatMeanclicked(void)
{
	if(!inverse)
		core.StatMean(0);
	else
	{
		pbInv->setOn(false);
		core.StatSumSquares(0);
	}

	UpdateDisplay(true);
}

void Calculator::slotStatStdDevclicked(void)
{
	if(inverse)
	{
		// std (n-1)
		core.StatStdDeviation(0);
		pbInv->setOn(false);
	}
	else
	{
		// std (n)
		core.StatStdSample(0);
	}

	UpdateDisplay(true);
}

void Calculator::slotStatMedianclicked(void)
{
	if(!inverse)
	{
		// std (n-1)
		core.StatMedian(0);
	}
	else
	{
		// std (n)
		core.StatMedian(0);
		pbInv->setOn(false);
	}
	// it seems two different modes should be implemented, but...?
	UpdateDisplay(true);
}

void Calculator::slotStatDataInputclicked(void)
{
	if(!inverse)
	{
		core.StatDataNew(calc_display->getAmount());
	}
	else
	{
		pbInv->setOn(false);
		core.StatDataDel(0);
		//statusBar()->message(TQString("Last stat item erased"), 3000);
	}

	UpdateDisplay(true);
}

void Calculator::slotStatClearDataclicked(void)
{
        if(!inverse)
	{
		core.StatClearAll(0);
		//statusBar()->message(TQString("Stat mem cleared"), 3000);
	}
	else
	{
		pbInv->setOn(false);
		UpdateDisplay(false);
	}
}

void Calculator::slotConstclicked(int button)
{
	if (mConverterPage && mConverterPage->isVisible()) {
		double val_double = pbConstant[button]->constant().toDouble();
		TQString targetStr = formatDouble(val_double);
		if (m_convActiveSlot == 1) {
			m_convInput1 = targetStr;
			setVal1Text(targetStr);
		} else {
			m_convInput2 = targetStr;
			setVal2Text(targetStr);
		}
		recalculateConversion();
		return;
	}

	if(!inverse)
	{
		//set the display to the configured value of Constant Button
		calc_display->setAmount(pbConstant[button]->constant());
	}
	else
	{
		pbInv->setOn(false);
		//CalcSettings::setValueConstant(button, calc_display->text());
		// below set new tooltip
		pbConstant[button]->setLabelAndTooltip();
		// work around: after storing a number, pressing a digit should start
		// a new number
		calc_display->setAmount(calc_display->getAmount());
	}

	UpdateDisplay(false);
}

void Calculator::showSettings()
{
	SettingsDialog *dlg = new SettingsDialog(false, tqApp->desktop());
	connect(dlg, TQ_SIGNAL(settingsApplied()), this, TQ_SLOT(updateSettings()));
	if (dlg->exec() == TQDialog::Accepted) {
		updateSettings();
	}
	delete dlg;
}


// these 6 slots are just a quick hack, instead of setting the
// TextEdit fields in the configuration dialog, we are setting the
// Settingvalues themselves!!
void Calculator::slotChooseScientificConst0(int option)
{
//  (tmp_const->kcfg_valueConstant0)->setText(CalcConstMenu::Constants[option].value);
//  (tmp_const->kcfg_nameConstant0)->setText(CalcConstMenu::Constants[option].label);
}

void Calculator::slotChooseScientificConst1(int option)
{
//  (tmp_const->kcfg_valueConstant1)->setText(CalcConstMenu::Constants[option].value);
//  (tmp_const->kcfg_nameConstant1)->setText(CalcConstMenu::Constants[option].label);
}

void Calculator::slotChooseScientificConst2(int option)
{
//  (tmp_const->kcfg_valueConstant2)->setText(CalcConstMenu::Constants[option].value);
//  (tmp_const->kcfg_nameConstant2)->setText(CalcConstMenu::Constants[option].label);
}

void Calculator::slotChooseScientificConst3(int option)
{
//  (tmp_const->kcfg_valueConstant3)->setText(CalcConstMenu::Constants[option].value);
//  (tmp_const->kcfg_nameConstant3)->setText(CalcConstMenu::Constants[option].label);
}

void Calculator::slotChooseScientificConst4(int option)
{
//  (tmp_const->kcfg_valueConstant4)->setText(CalcConstMenu::Constants[option].value);
//  (tmp_const->kcfg_nameConstant4)->setText(CalcConstMenu::Constants[option].label);
}

void Calculator::slotChooseScientificConst5(int option)
{
//  (tmp_const->kcfg_valueConstant5)->setText(CalcConstMenu::Constants[option].value);
//  (tmp_const->kcfg_nameConstant5)->setText(CalcConstMenu::Constants[option].label);
}


void Calculator::slotStatshow(bool toggled)
{
	if(toggled)
	{
		pbStat["NumData"]->show();
		pbStat["Mean"]->show();
		pbStat["StandardDeviation"]->show();
		pbStat["Median"]->show();
		pbStat["InputData"]->show();
		pbStat["ClearData"]->show();
	}
	else
	{
		pbStat["NumData"]->hide();
		pbStat["Mean"]->hide();
		pbStat["StandardDeviation"]->hide();
		pbStat["Median"]->hide();
		pbStat["InputData"]->hide();
		pbStat["ClearData"]->hide();
	}
	adjustSize();
	// setFixedSize(sizeHint());
	
	updateModeVisibility();
}

void Calculator::slotScientificshow(bool toggled)
{
	if(toggled)
	{
	        pbScientific["HypMode"]->show();
		pbScientific["Sine"]->show();
		pbScientific["Cosine"]->show();
		pbScientific["Tangent"]->show();
		pbScientific["Log10"]->show();
		pbScientific["LogNatural"]->show();
		pbAngleChoose->show();
		//if(!statusBar()->hasItem(2))
		//	statusBar()->insertFixedItem(" DEG ", 2, true);
		//statusBar()->setItemAlignment(2, AlignCenter);
		calc_display->setStatusText(2, "Deg");
		slotAngleSelected(0);
	}
	else
	{
	        pbScientific["HypMode"]->hide();
		pbScientific["Sine"]->hide();
		pbScientific["Cosine"]->hide();
		pbScientific["Tangent"]->hide();
		pbScientific["Log10"]->hide();
		pbScientific["LogNatural"]->hide();
		pbAngleChoose->hide();
		//if(statusBar()->hasItem(2))
		//	statusBar()->removeItem(2);
		calc_display->setStatusText(2, TQString());
	}
	adjustSize();
	// setFixedSize(sizeHint());
	
	updateModeVisibility();
}

void Calculator::slotLogicshow(bool toggled)
{
	if(toggled)
	{
	        pbLogic["AND"]->show();
		pbLogic["OR"]->show();
		pbLogic["XOR"]->show();
		pbLogic["One-Complement"]->show();
		pbLogic["LeftShift"]->show();
		pbLogic["RightShift"]->show();
		//if(!statusBar()->hasItem(1))
		//	statusBar()->insertFixedItem(" HEX ", 1, true);
		//statusBar()->setItemAlignment(1, AlignCenter);
		calc_display->setStatusText(1, "Hex");
		resetBase();
		BaseChooseGroup->show();
		for (int i=10; i<16; i++)
			(NumButtonGroup->find(i))->show();
	}
	else
	{
	    pbLogic["AND"]->hide();
		pbLogic["OR"]->hide();
		pbLogic["XOR"]->hide();
		pbLogic["One-Complement"]->hide();
		pbLogic["LeftShift"]->hide();
		pbLogic["RightShift"]->hide();
		// Hide Hex-Buttons, but first switch back to decimal
		resetBase();
		BaseChooseGroup->hide();
		//if(statusBar()->hasItem(1))
		//	statusBar()->removeItem(1);
		calc_display->setStatusText(1, TQString());
		for (int i=10; i<16; i++)
			(NumButtonGroup->find(i))->hide();
	}
	adjustSize();
	// setFixedSize(sizeHint());
	
	updateModeVisibility();
}

void Calculator::updateModeVisibility()
{
	if (m_historyWindow) {
		m_historyWindow->close();
	}

	// Save state of current converter mode if we are switching away from it
	if (m_currentLoadedModeIdx >= 0 && m_currentLoadedModeIdx < 13) {
		m_convSelectedUnit1[m_currentLoadedModeIdx] = m_convUnit1;
		m_convSelectedUnit2[m_currentLoadedModeIdx] = m_convUnit2;
		m_convSelectedInput1[m_currentLoadedModeIdx] = m_convInput1;
		m_convSelectedInput2[m_currentLoadedModeIdx] = m_convInput2;
		m_convSelectedActiveSlot[m_currentLoadedModeIdx] = m_convActiveSlot;
	}

	bool is_scientific = (_calc_mode == ModeScientific);
	bool is_logic = (_calc_mode == ModeProgrammer);
	bool is_stat = (_calc_mode == ModeStatistics);
	bool is_standard = (_calc_mode == ModeStandard);
	bool is_datecalc = (_calc_mode == ModeDateCalc);
	bool is_volume = (_calc_mode >= ModeVolume);

	if (is_volume) {
		int newIdx = _calc_mode - ModeVolume;
		m_convUnit1 = m_convSelectedUnit1[newIdx];
		m_convUnit2 = m_convSelectedUnit2[newIdx];
		m_convInput1 = m_convSelectedInput1[newIdx];
		m_convInput2 = m_convSelectedInput2[newIdx];
		m_convActiveSlot = m_convSelectedActiveSlot[newIdx];
		m_currentLoadedModeIdx = newIdx;

		// Populate unit choices in comboboxes
		populateConverterUnits(_calc_mode);

		// Synchronize display text
		setVal1Text(m_convInput1);
		setVal2Text(m_convInput2);
	} else {
		m_currentLoadedModeIdx = -1;
	}

	if (pbConstantsMenu) {
		if (is_logic || is_datecalc) {
			pbConstantsMenu->hide();
			pbConstantsMenu->setEnabled(false);
		} else {
			pbConstantsMenu->show();
			pbConstantsMenu->setEnabled(true);
		}
	}

	// Set dynamic resizability constraints for modern modes
	if (is_standard || is_scientific || is_logic || is_datecalc || is_volume) {
		int minH = is_volume ? 430 : 400;
		setMinimumSize(350, minH);
		if (centralWidget()) centralWidget()->setMinimumSize(350, minH);
		setMaximumSize(TQWIDGETSIZE_MAX, TQWIDGETSIZE_MAX);
		set_colors();

		// Hide memory frames on mode switch
		m_programmerMemoryFrameVisible = false;
		if (mProgrammerMemoryFrame) mProgrammerMemoryFrame->hide();
		setProgrammerGridVisible(true);
		if (btnProgM) btnProgM->setText(TQString::fromUtf8("M\xe2\x96\xbc"));

		if (is_standard) {
			lblModeTitle->setText(tr_str("Standard"));
			mStandardPage->show();
			mStandardPageGrid->show();
			
			if (mScientificPage) mScientificPage->hide();
			if (mScientificPageGrid) mScientificPageGrid->hide();
			if (mProgrammerPage) mProgrammerPage->hide();
			if (mProgrammerPageGrid) mProgrammerPageGrid->hide();
			if (mDateCalcPage) mDateCalcPage->hide();
			if (mConverterPage) mConverterPage->hide();
			if (calc_display) calc_display->show();
		} else if (is_scientific) {
			lblModeTitle->setText(tr_str("Scientific"));
			mStandardPage->show(); // Memory buttons
			if (mScientificPage) mScientificPage->show();
			if (mScientificPageGrid) mScientificPageGrid->show();
			
			mStandardPageGrid->hide();
			if (mProgrammerPage) mProgrammerPage->hide();
			if (mProgrammerPageGrid) mProgrammerPageGrid->hide();
			if (mDateCalcPage) mDateCalcPage->hide();
			if (mConverterPage) mConverterPage->hide();
			if (calc_display) calc_display->show();
		} else if (is_logic) {
			lblModeTitle->setText(tr_str("Programmer"));
			mStandardPage->hide(); // Hide memory buttons in programmer mode
			if (mProgrammerPage) mProgrammerPage->show();
			if (mProgrammerPageGrid) mProgrammerPageGrid->show();
			
			mStandardPageGrid->hide();
			if (mScientificPage) mScientificPage->hide();
			if (mScientificPageGrid) mScientificPageGrid->hide();
			if (mDateCalcPage) mDateCalcPage->hide();
			if (mConverterPage) mConverterPage->hide();
			if (calc_display) calc_display->show();
		} else if (is_datecalc) {
			lblModeTitle->setText(tr_str("Date calculation"));
			if (!mDateCalcPage) {
				setupDateCalcPage(static_cast<TQWidget*>(centralWidget()));
				TQVBoxLayout *ml = dynamic_cast<TQVBoxLayout*>(centralWidget()->layout());
				if (ml) {
					ml->insertWidget(2, mDateCalcPage, 1);
				}
				set_colors();
			}
			if (mDateCalcPage) {
				mDateCalcPage->show();
				updateDateCalcPageSizes();
			}

			// Hide everything else
			mStandardPage->hide();
			mStandardPageGrid->hide();
			if (mScientificPage) mScientificPage->hide();
			if (mScientificPageGrid) mScientificPageGrid->hide();
			if (mProgrammerPage) mProgrammerPage->hide();
			if (mProgrammerPageGrid) mProgrammerPageGrid->hide();
			if (mConverterPage) mConverterPage->hide();
			if (calc_display) calc_display->hide();
		} else if (is_volume) {
			lblModeTitle->setText(converterTitle(_calc_mode));
			if (!mConverterPage) {
				setupConverterPage(static_cast<TQWidget*>(centralWidget()));
				TQVBoxLayout *ml = dynamic_cast<TQVBoxLayout*>(centralWidget()->layout());
				if (ml) {
					ml->insertWidget(2, mConverterPage, 1);
				}
				set_colors();
			}
			if (mConverterPage) {
				mConverterPage->show();
				updateConverterSizes();
			}

			// Hide everything else
			mStandardPage->hide();
			mStandardPageGrid->hide();
			if (mScientificPage) mScientificPage->hide();
			if (mScientificPageGrid) mScientificPageGrid->hide();
			if (mProgrammerPage) mProgrammerPage->hide();
			if (mProgrammerPageGrid) mProgrammerPageGrid->hide();
			if (mDateCalcPage) mDateCalcPage->hide();
			if (calc_display) calc_display->hide();
		}

		// Hide legacy pages
		mSmallPage->hide();
		mNumericPage->hide();
		mLargePage->hide();

		// Hide programmer settings (except in programmer mode)
		if (!is_logic) BaseChooseGroup->hide();
		pbAngleChoose->hide();
		pbInv->hide();

		if (centralWidget() && centralWidget()->layout()) {
			calc_display->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);
			calc_display->setMinimumHeight(0);
			calc_display->setMaximumHeight(TQWIDGETSIZE_MAX);
			
			// Use a very strong stretch bias towards the button grid
			TQVBoxLayout *ml = (TQVBoxLayout*)centralWidget()->layout();
			ml->setStretchFactor(calc_display, (is_datecalc || is_volume) ? 0 : 2);
			if (is_scientific) {
				ml->setStretchFactor(mScientificPage, 1);
			} else if (is_logic) {
				ml->setStretchFactor(mProgrammerPage, 0);
			}
			ml->setStretchFactor(mStandardPage, 1);
			
			if (is_standard) {
				ml->setStretchFactor(mStandardPageGrid, 12);
			} else if (is_scientific) {
				ml->setStretchFactor(mScientificPageGrid, 12);
			} else if (is_logic) {
				ml->setStretchFactor(mProgrammerPageGrid, 12);
			} else if (is_datecalc) {
				ml->setStretchFactor(mDateCalcPage, 12);
			} else if (is_volume) {
				ml->setStretchFactor(mConverterPage, 12);
			}
		}
		
		minH = is_volume ? 430 : 400;
		setMinimumSize(350, minH);
		if (centralWidget()) centralWidget()->setMinimumSize(350, minH);
	} else {
		// Restore default size policy and allow resizing for legacy modes
		setMinimumSize(350, 400);
		if (centralWidget()) centralWidget()->setMinimumSize(350, 400);
		setMaximumSize(TQWIDGETSIZE_MAX, TQWIDGETSIZE_MAX);

			lblModeTitle->setText(tr_str("Statistics"));

		mStandardPage->hide();
		mStandardPageGrid->hide();
		if (mScientificPage) mScientificPage->hide();
		if (mScientificPageGrid) mScientificPageGrid->hide();
		if (mProgrammerPage) mProgrammerPage->hide();
		if (mProgrammerPageGrid) mProgrammerPageGrid->hide();
		if (mDateCalcPage) mDateCalcPage->hide();
		if (mConverterPage) mConverterPage->hide();
		if (calc_display) calc_display->show();

		// Show corresponding pages
		mNumericPage->show();
		mLargePage->show();
		
		// Small page is visible if stat mode is on
		mSmallPage->setShown(is_stat);

		adjustSize();
		// setFixedSize(sizeHint());
	}

	if (pbHistory) {
		if (is_volume || is_datecalc) {
			pbHistory->hide();
		} else {
			pbHistory->show();
		}
	}
}

void Calculator::slotConstantsShow(bool toggled)
{
	if(toggled)
	{
		pbConstant[0]->show();
		pbConstant[1]->show();
		pbConstant[2]->show();
		pbConstant[3]->show();
		pbConstant[4]->show();
		pbConstant[5]->show();

	}
	else
	{
		pbConstant[0]->hide();
		pbConstant[1]->hide();
		pbConstant[2]->hide();
		pbConstant[3]->hide();
		pbConstant[4]->hide();
		pbConstant[5]->hide();
	}
	adjustSize();
	// setFixedSize(sizeHint());
	
}

// This function is for setting the constant names configured in the
// calc settings menu. If the user doesn't enter a name for the
// constant C1 to C6 is used.
void Calculator::changeButtonNames()
{
	pbConstant[0]->setLabelAndTooltip();
	pbConstant[1]->setLabelAndTooltip();
	pbConstant[2]->setLabelAndTooltip();
	pbConstant[3]->setLabelAndTooltip();
	pbConstant[4]->setLabelAndTooltip();
	pbConstant[5]->setLabelAndTooltip();
}

void Calculator::slotShowAll(void)
{
	/*if(!actionStatshow->isChecked()) actionStatshow->setOn(true);
	if(!actionScientificshow->isChecked()) actionScientificshow->setOn(true);
	if(!actionLogicshow->isChecked()) actionLogicshow->setOn(true);
	if(!actionConstantsShow->isChecked()) actionConstantsShow->setOn(true);*/
}

void Calculator::slotHideAll(void)
{
	/*if(actionStatshow->isChecked()) actionStatshow->setOn(false);
	if(actionScientificshow->isChecked()) actionScientificshow->setOn(false);
	if(actionLogicshow->isChecked()) actionLogicshow->setOn(false);
	if(actionConstantsShow->isChecked()) actionConstantsShow->setOn(false);*/
}

void Calculator::retranslateUi()
{
	// 1. Window title
	setCaption(tr_str("Calculator"));

	// 2. Menu Bar items
	if (menuBar()) {
		menuBar()->changeItem(10, tr_str("&Display"));
		menuBar()->changeItem(11, tr_str("&Settings"));
	}

	// 3. Populate displayMenu items
	if (displayMenu) {
		displayMenu->clear();
		displayMenu->insertItem(new CalcMenuItem(standard_png, standard_png_len, tr_str("Standard"), _calc_mode == ModeStandard, true), 100);
		displayMenu->insertItem(new CalcMenuItem(science_png, science_png_len, tr_str("Scientific"), _calc_mode == ModeScientific, true), 101);
		displayMenu->insertItem(new CalcMenuItem(coder_png, coder_png_len, tr_str("Programmer"), _calc_mode == ModeProgrammer, true), 102);
		displayMenu->insertItem(new CalcMenuItem(date_png, date_png_len, tr_str("Date calculation"), _calc_mode == ModeDateCalc, true), 103);
		displayMenu->insertSeparator();
		displayMenu->insertItem(new CalcMenuItem(volume_png, volume_png_len, tr_str("Volume"), _calc_mode == ModeVolume, true), 200);
		displayMenu->insertItem(new CalcMenuItem(lenght_png, lenght_png_len, tr_str("Length"), _calc_mode == ModeLength, true), 201);
		displayMenu->insertItem(new CalcMenuItem(mass_png, mass_png_len, tr_str("Weight and mass"), _calc_mode == ModeMass, true), 202);
		displayMenu->insertItem(new CalcMenuItem(temp_png, temp_png_len, tr_str("Temperature"), _calc_mode == ModeTemp, true), 203);
		displayMenu->insertItem(new CalcMenuItem(energy_png, energy_png_len, tr_str("Energy"), _calc_mode == ModeEnergy, true), 204);
		displayMenu->insertItem(new CalcMenuItem(area_png, area_png_len, tr_str("Area"), _calc_mode == ModeArea, true), 205);
		displayMenu->insertItem(new CalcMenuItem(speed_png, speed_png_len, tr_str("Speed"), _calc_mode == ModeSpeed, true), 206);
		displayMenu->insertItem(new CalcMenuItem(time_png, time_png_len, tr_str("Time"), _calc_mode == ModeTime, true), 207);
		displayMenu->insertItem(new CalcMenuItem(power_png, power_png_len, tr_str("Power"), _calc_mode == ModePower, true), 208);
		displayMenu->insertItem(new CalcMenuItem(data_png, data_png_len, tr_str("Data"), _calc_mode == ModeData, true), 209);
		displayMenu->insertItem(new CalcMenuItem(pressure_png, pressure_png_len, tr_str("Pressure"), _calc_mode == ModePressure, true), 210);
		displayMenu->insertItem(new CalcMenuItem(angle_png, angle_png_len, tr_str("Angle"), _calc_mode == ModeAngle, true), 211);
		displayMenu->insertItem(new CalcMenuItem(roman_png, roman_png_len, tr_str("Roman numerals"), _calc_mode == ModeRoman, true), 212);
	}

	// 4. Populate settingsMenu items
	if (settingsMenu) {
		settingsMenu->clear();
		settingsMenu->insertItem(tr_str("Configure Calculator"), this, TQ_SLOT(slotMenuConfigure()), 0, 300);
		settingsMenu->insertItem(tr_str("About"), this, TQ_SLOT(slotMenuAbout()), 0, 301);
	}

	// 5. Title of the mode shown in the header bar
	if (lblModeTitle) {
		if (_calc_mode == ModeStandard) lblModeTitle->setText(tr_str("Standard"));
		else if (_calc_mode == ModeScientific) lblModeTitle->setText(tr_str("Scientific"));
		else if (_calc_mode == ModeProgrammer) lblModeTitle->setText(tr_str("Programmer"));
		else if (_calc_mode == ModeDateCalc) lblModeTitle->setText(tr_str("Date calculation"));
		else lblModeTitle->setText(converterTitle(_calc_mode));
	}

	// 6. Date calculation labels/combo boxes
	if (mDateModeCombo) {
		mDateModeCombo->blockSignals(true);
		int current = mDateModeCombo->currentItem();
		mDateModeCombo->clear();
		mDateModeCombo->insertItem(tr_str("Difference between dates"));
		mDateModeCombo->insertItem(tr_str("Add or subtract days"));
		mDateModeCombo->setCurrentItem(current);
		mDateModeCombo->blockSignals(false);
	}
	if (lblFrom) lblFrom->setText(tr_str("From"));
	if (lblTo) lblTo->setText(tr_str("To"));
	if (mAddRadio) mAddRadio->setText(tr_str("Add"));
	if (mSubRadio) mSubRadio->setText(tr_str("Subtract"));
	if (lblYears) lblYears->setText(tr_str("years"));
	if (lblMonths) lblMonths->setText(tr_str("months"));
	if (lblDays) lblDays->setText(tr_str("days"));
	if (mDateDiffLabel) {
		if (mDateModeCombo && mDateModeCombo->currentItem() == 1) {
			mDateDiffLabel->setText(tr_str("Date"));
		} else {
			mDateDiffLabel->setText(tr_str("Difference"));
		}
	}

	// Update date difference output to refresh strings
	updateDateDiffResult();

	// 7. Unit Converter combo boxes
	if (_calc_mode >= ModeVolume) {
		populateConverterUnits(_calc_mode);
		recalculateConversion();
	}
}

void Calculator::updateSettings()
{
	Translation::reload();
	retranslateUi();

	changeButtonNames();
	set_colors(true);
	set_precision();
	// Show the result in the app's caption in taskbar (wishlist - bug #52858)
	disconnect(calc_display, TQ_SIGNAL(changedText(const TQString &)),
		   this, 0);
	if (false)
	{
		connect(calc_display,
			TQ_SIGNAL(changedText(const TQString &)),
			TQ_SLOT(setCaption(const TQString &)));
	}
	else
	{
		setCaption(tr_str("Calculator"));
	}
	calc_display->changeSettings();

	updateGeometry();

	//
	// 1999-10-31 Espen Sand: Don't ask me why ;)
	//
	tqApp->processEvents();
}

// ============================================================
// Expression Display helpers (Standard mode, sequential calc)
// ============================================================

TQString Calculator::exprFormatNumber(const KNumber &n) const
{
	if (_calc_mode == ModeProgrammer && calc_display->base() != NB_DECIMAL) {
		unsigned long long int tmp_workaround = static_cast<unsigned long long int>(n);
		if (_word_size == 1) tmp_workaround &= 0xFFFFFFFFull;
		else if (_word_size == 2) tmp_workaround &= 0xFFFFull;
		else if (_word_size == 3) tmp_workaround &= 0xFFull;
		return TQString::number(tmp_workaround, calc_display->base()).upper();
	}

	TQString s = n.toTQString(9);
	// Remove trailing zeros after decimal point for cleaner display
	if (s.contains('.')) {
		while (s.endsWith("0"))
			s.truncate(s.length() - 1);
		if (s.endsWith("."))
			s.truncate(s.length() - 1);
	}
	return s;
}

void Calculator::exprBinaryOp(const TQString &op_symbol)
{
	if (_expr_just_equaled) {
		// After '=', start fresh expression with the result
		_expr_text = exprFormatNumber(calc_display->getAmount()) + " " + op_symbol + " ";
		_expr_just_equaled = false;
		_expr_op_pending = true;
		_expr_unary = TQString();
		return;
	}

	if (_expr_op_pending) {
		// Operator replacement: strip old op and put new one
		// e.g. "5 + " -> "5 - "
		_expr_text = _expr_text.stripWhiteSpace();
		// Remove last character (the old operator symbol)
		_expr_text.truncate(_expr_text.length() - 1);
		_expr_text = _expr_text.stripWhiteSpace();
		_expr_text += " " + op_symbol + " ";
		return;
	}

	// Normal case: append current operand then operator
	TQString operand_str;
	if (!_expr_unary.isEmpty()) {
		operand_str = _expr_unary;
		_expr_unary = TQString();
	} else {
		operand_str = exprFormatNumber(calc_display->getAmount());
	}

	// Accumulate the expression (like Windows calculator does)
	_expr_text += operand_str + " " + op_symbol + " ";
	_expr_op_pending = true;
}

void Calculator::exprUnaryOp(const TQString &func_name)
{
	_expr_just_equaled = false;

	TQString operand_str;
	if (!_expr_unary.isEmpty()) {
		operand_str = _expr_unary;
	} else {
		operand_str = exprFormatNumber(calc_display->getAmount());
	}

	// Wrap: func_name(operand) — supports nesting like sqrt(sqrt(9))
	_expr_unary = func_name + "(" + operand_str + ")";
}

void Calculator::UpdateDisplay(bool get_amount_from_core,
				bool store_result_in_history)
{
	if(get_amount_from_core)
	{
		calc_display->update_from_core(core, store_result_in_history);
	}
	else
	{
		calc_display->update();
	}

	updateBitboard();

	calc_display->setExpression(_expr_text + _expr_unary);

	pbInv->setOn(false);

}


static TQPalette createButtonPalette(const TQPalette &basePal, const TQColor &textCol, const TQColor &bgCol)
{
	TQPalette pal = basePal;
	pal.setColor(TQPalette::Active, TQColorGroup::Button, bgCol);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Button, bgCol);
	pal.setColor(TQPalette::Active, TQColorGroup::Background, bgCol);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Background, bgCol);
	pal.setColor(TQPalette::Active, TQColorGroup::ButtonText, textCol);
	pal.setColor(TQPalette::Inactive, TQColorGroup::ButtonText, textCol);
	pal.setColor(TQPalette::Active, TQColorGroup::Foreground, textCol);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, textCol);
	return pal;
}

static TQPalette getButtonPalette(CalcButton *btn, const TQPalette &winPal,
                                  const TQColor &colorNumKeyFg, const TQColor &colorNumKeyBg,
                                  const TQColor &colorOpKeyFg, const TQColor &colorOpKeyBg,
                                  const TQColor &colorClearKeyFg, const TQColor &colorClearKeyBg,
                                  const TQColor &colorMemKeyFg, const TQColor &colorMemKeyBg,
                                  const TQColor &colorEqualBg, const TQColor &colorEqualFg,
                                  const TQColor &colorPanelBg, const TQColor &colorFg,
                                  const TQColor &colorBg)
{
	CalcButton::ButtonType type = btn->buttonType();
	TQString name = btn->name();

	if (name == "pbProgMemoryDelete") {
		return createButtonPalette(winPal, colorFg, colorPanelBg);
	}

	if (type == CalcButton::TypeHeader || name.contains("Minimize") || name.contains("Maximize") || name.contains("Close") || name.contains("Pin") || name.contains("Hamburger") || name.contains("History") || name == "pbPin" || name == "pbMinimize" || name == "pbMaximize" || name == "pbClose" || name == "pbHamburger" || name == "pbHistory" || name == "DatePeriodPickerButton") {
		return createButtonPalette(winPal, colorFg, colorBg);
	}

	if (name == "pbEqual" || name == "pbStdEqual" || type == CalcButton::TypeEqual) {
		return createButtonPalette(winPal, colorEqualFg, colorEqualBg);
	}

	if (type == CalcButton::TypeDigit) {
		return createButtonPalette(winPal, colorNumKeyFg, colorNumKeyBg);
	}

	if (type == CalcButton::TypeMemory || name == "pbMR" || name == "pbMC" || name == "pbMS" || name == "pbMPlus" || name == "pbMMinus" || name.contains("Mem")) {
		return createButtonPalette(winPal, colorMemKeyFg, colorMemKeyBg);
	}

	if (name.contains("Clear") || name.contains("CE") || name.contains("BackSpace") || name.contains("BS") || name.contains("Backspace") || name == "pbStdCE" || name == "pbClear" || name == "pbStdC" || name == "pbSciC" || name == "pbProgC" || (name.contains("C-Button") && name != "C-Button")) {
		return createButtonPalette(winPal, colorClearKeyFg, colorClearKeyBg);
	}

	if (type == CalcButton::TypeOperator || name == "pbPlus" || name == "pbMinus" || name == "pbX" || name == "pbDivision" || name == "pbMod" || name == "pbPercent") {
		return createButtonPalette(winPal, colorOpKeyFg, colorOpKeyBg);
	}

	return createButtonPalette(winPal, colorMemKeyFg, colorMemKeyBg);
}

void Calculator::set_colors(bool clear_cache)
{
	TQPushButton *p = NULL;

	calc_display->changeSettings();

	TDEConfig *config = new TDEConfig("calcrc");
	config->reparseConfiguration();
	config->setGroup("Preferences");
	int mode = config->readNumEntry("AppearanceMode", 2); // default to Classic
	TQString selectedTheme = config->readEntry("SelectedTheme", "Midnight Blue");
	int keyFontType = 1; // Default to System
	TQFont keyFont;

	TQColor defaultBg(230, 230, 230);
	TQColor defaultFg(0, 0, 0);
	TQColor defaultSmallFg(0, 0, 0);
	TQColor defaultNumKeyFg(0, 0, 0);
	TQColor defaultNumKeyBg(255, 255, 255);
	TQColor defaultOtherKeyFg(0, 0, 0);
	TQColor defaultOtherKeyBg(230, 230, 230);
	TQColor defaultEqualBg(0, 103, 192);
	TQColor defaultEqualFg(255, 255, 255);
	TQColor defaultKeysBorders(0, 0, 0);
	TQColor defaultWindowBorder(0, 0, 0);
	TQColor defaultDisplayBorder(0, 0, 0);
	TQColor defaultDisplayFg(0, 0, 0);
	TQColor defaultDisplayBg(189, 255, 180);
	TQColor defaultPanelBg(230, 230, 230);
	TQColor defaultMenuBg(230, 230, 230);

	TQColor colorBg, colorFg, colorSmallFg, colorNumKeyFg, colorNumKeyBg, colorOpKeyFg, colorOpKeyBg, colorClearKeyFg, colorClearKeyBg, colorMemKeyFg, colorMemKeyBg, colorEqualBg, colorEqualFg, colorDisplayFg, colorDisplayBg, colorPanelBg, keysBordersColor, windowBorderColor, displayBorderColor, colorMenuBg;
	int displayType = 0;
	TQFont defaultF("Courier New", 12);
	TQFont displayFont = defaultF;
	bool noDeco = false;
	bool standardMenuBar = false;
	bool invertIcons = false;
	bool keysBorders = false;
	bool windowBorder = false;
	bool displayBorder = false;
	bool boldIcons = false;

	if (mode == 0) { // Classic Dark
		colorBg = TQColor(0, 0, 0);
		colorFg = TQColor(255, 255, 255);
		colorSmallFg = TQColor(222, 222, 222);
		colorNumKeyFg = TQColor(255, 255, 255);
		colorNumKeyBg = TQColor(0, 0, 0);
		colorOpKeyFg = TQColor(255, 255, 255);
		colorOpKeyBg = TQColor(0, 0, 0);
		colorClearKeyFg = TQColor(255, 255, 255);
		colorClearKeyBg = TQColor(0, 0, 0);
		colorMemKeyFg = TQColor(255, 255, 255);
		colorMemKeyBg = TQColor(0, 0, 0);
		colorEqualBg = TQColor(0, 103, 192);
		colorEqualFg = TQColor(255, 255, 255);
		displayType = 0;
		keyFontType = 0;
		colorDisplayFg = TQColor(255, 255, 255);
		colorDisplayBg = TQColor(0, 0, 0);
		colorPanelBg = TQColor(62, 62, 62);
		colorMenuBg = TQColor(44, 44, 44);
		noDeco = false;
		standardMenuBar = false;
		invertIcons = true;
		keysBorders = false;
		keysBordersColor = TQColor(0, 0, 0);
		windowBorder = false;
		windowBorderColor = TQColor(0, 0, 0);
		displayBorder = false;
		displayBorderColor = TQColor(0, 0, 0);
		boldIcons = false;
	} else if (mode == 2) { // Classic
		colorBg = TQColor(242, 242, 242);
		colorFg = TQColor(0, 0, 0);
		colorSmallFg = TQColor(71, 71, 71);
		colorNumKeyFg = TQColor(0, 0, 0);
		colorNumKeyBg = TQColor(255, 255, 255);
		colorOpKeyFg = TQColor(0, 0, 0);
		colorOpKeyBg = TQColor(242, 242, 242);
		colorClearKeyFg = TQColor(0, 0, 0);
		colorClearKeyBg = TQColor(242, 242, 242);
		colorMemKeyFg = TQColor(0, 0, 0);
		colorMemKeyBg = TQColor(242, 242, 242);
		colorEqualBg = TQColor(0, 103, 192);
		colorEqualFg = TQColor(255, 255, 255);
		displayType = 0;
		keyFontType = 0;
		colorDisplayFg = TQColor(0, 0, 0);
		colorDisplayBg = TQColor(242, 242, 242);
		colorPanelBg = TQColor(231, 231, 231);
		colorMenuBg = TQColor(230, 230, 230);
		noDeco = false;
		standardMenuBar = false;
		invertIcons = false;
		keysBorders = false;
		keysBordersColor = TQColor(0, 0, 0);
		windowBorder = false;
		windowBorderColor = TQColor(0, 0, 0);
		displayBorder = false;
		displayBorderColor = TQColor(0, 0, 0);
		boldIcons = false;
	} else if (mode == 3) { // Theme
		if (selectedTheme == "Midnight Blue") {
			colorBg = TQColor(26, 37, 54);
			colorFg = TQColor(255, 255, 255);
			colorSmallFg = TQColor(160, 170, 181);
			colorNumKeyFg = TQColor(255, 255, 255);
			colorNumKeyBg = TQColor(46, 61, 82);
			colorOpKeyFg = TQColor(255, 255, 255);
			colorOpKeyBg = TQColor(35, 47, 66);
			colorClearKeyFg = TQColor(255, 255, 255);
			colorClearKeyBg = TQColor(35, 47, 66);
			colorMemKeyFg = TQColor(255, 255, 255);
			colorMemKeyBg = TQColor(35, 47, 66);
			colorEqualBg = TQColor(74, 144, 226);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 3;
			keyFontType = 0;
			colorDisplayFg = TQColor(0, 255, 204);
			colorDisplayBg = TQColor(13, 21, 32);
			colorPanelBg = TQColor(46, 61, 82);
			colorMenuBg = TQColor(46, 61, 82);
			noDeco = false;
			standardMenuBar = false;
			invertIcons = true;
			keysBorders = false;
			keysBordersColor = TQColor(0, 0, 0);
			windowBorder = false;
			windowBorderColor = TQColor(0, 0, 0);
			displayBorder = false;
			displayBorderColor = TQColor(0, 0, 0);
			boldIcons = false;
		} else if (selectedTheme == "Forest Green") {
			colorBg = TQColor(28, 46, 36);
			colorFg = TQColor(230, 242, 235);
			colorSmallFg = TQColor(141, 163, 151);
			colorNumKeyFg = TQColor(230, 242, 235);
			colorNumKeyBg = TQColor(42, 66, 53);
			colorOpKeyFg = TQColor(230, 242, 235);
			colorOpKeyBg = TQColor(33, 54, 42);
			colorClearKeyFg = TQColor(230, 242, 235);
			colorClearKeyBg = TQColor(33, 54, 42);
			colorMemKeyFg = TQColor(230, 242, 235);
			colorMemKeyBg = TQColor(33, 54, 42);
			colorEqualBg = TQColor(46, 139, 87);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 7;
			keyFontType = 0;
			colorDisplayFg = TQColor(57, 255, 20);
			colorDisplayBg = TQColor(11, 19, 14);
			colorPanelBg = TQColor(42, 66, 53);
			colorMenuBg = TQColor(42, 66, 53);
			noDeco = false;
			standardMenuBar = false;
			invertIcons = true;
			keysBorders = false;
			keysBordersColor = TQColor(0, 0, 0);
			windowBorder = false;
			windowBorderColor = TQColor(0, 0, 0);
			displayBorder = false;
			displayBorderColor = TQColor(0, 0, 0);
			boldIcons = false;
		} else if (selectedTheme == "Classic Gray") {
			colorBg = TQColor(204, 204, 204);
			colorFg = TQColor(0, 0, 0);
			colorSmallFg = TQColor(203, 203, 203);
			colorNumKeyFg = TQColor(0, 0, 0);
			colorNumKeyBg = TQColor(221, 221, 221);
			colorOpKeyFg = TQColor(0, 0, 0);
			colorOpKeyBg = TQColor(192, 192, 192);
			colorClearKeyFg = TQColor(0, 0, 0);
			colorClearKeyBg = TQColor(192, 192, 192);
			colorMemKeyFg = TQColor(0, 0, 0);
			colorMemKeyBg = TQColor(192, 192, 192);
			colorEqualBg = TQColor(128, 128, 128);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 8;
			keyFontType = 0;
			colorDisplayFg = TQColor(242, 242, 242);
			colorDisplayBg = TQColor(126, 126, 126);
			colorPanelBg = TQColor(133, 133, 133);
			colorMenuBg = TQColor(221, 221, 221);
			noDeco = false;
			standardMenuBar = false;
			invertIcons = false;
			keysBorders = false;
			keysBordersColor = TQColor(0, 0, 0);
			windowBorder = false;
			windowBorderColor = TQColor(0, 0, 0);
			displayBorder = false;
			displayBorderColor = TQColor(0, 0, 0);
			boldIcons = true;
		} else if (selectedTheme == "Coloured") {
			colorBg = TQColor(93, 115, 215);
			colorFg = TQColor(255, 255, 255);
			colorSmallFg = TQColor(130, 228, 255);
			colorNumKeyFg = TQColor(255, 255, 255);
			colorNumKeyBg = TQColor(19, 121, 16);
			colorOpKeyFg = TQColor(255, 255, 255);
			colorOpKeyBg = TQColor(95, 160, 51);
			colorClearKeyFg = TQColor(255, 255, 255);
			colorClearKeyBg = TQColor(176, 32, 34);
			colorMemKeyFg = TQColor(255, 255, 255);
			colorMemKeyBg = TQColor(26, 56, 115);
			colorEqualBg = TQColor(210, 188, 45);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 6;
			keyFontType = 0;
			colorDisplayFg = TQColor(255, 255, 255);
			colorDisplayBg = TQColor(11, 19, 14);
			colorPanelBg = TQColor(87, 108, 201);
			colorMenuBg = TQColor(36, 77, 110);
			noDeco = false;
			standardMenuBar = false;
			invertIcons = false;
			keysBorders = true;
			keysBordersColor = TQColor(84, 104, 194);
			windowBorder = true;
			windowBorderColor = TQColor(96, 119, 222);
			displayBorder = true;
			displayBorderColor = TQColor(46, 46, 46);
			boldIcons = true;
		} else if (selectedTheme == "orange style") {
			colorBg = TQColor(181, 111, 25);
			colorFg = TQColor(230, 242, 235);
			colorSmallFg = TQColor(201, 119, 18);
			colorNumKeyFg = TQColor(0, 0, 0);
			colorNumKeyBg = TQColor(249, 152, 34);
			colorOpKeyFg = TQColor(0, 0, 0);
			colorOpKeyBg = TQColor(224, 137, 31);
			colorClearKeyFg = TQColor(0, 0, 0);
			colorClearKeyBg = TQColor(165, 101, 23);
			colorMemKeyFg = TQColor(255, 255, 255);
			colorMemKeyBg = TQColor(176, 75, 17);
			colorEqualBg = TQColor(156, 59, 24);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 6;
			keyFontType = 0;
			colorDisplayFg = TQColor(253, 139, 63);
			colorDisplayBg = TQColor(44, 22, 16);
			colorPanelBg = TQColor(110, 52, 41);
			colorMenuBg = TQColor(181, 111, 25);
			noDeco = true;
			standardMenuBar = false;
			invertIcons = true;
			keysBorders = false;
			keysBordersColor = TQColor(0, 0, 0);
			windowBorder = true;
			windowBorderColor = TQColor(124, 79, 0);
			displayBorderColor = TQColor(0, 0, 0);
			boldIcons = false;
		} else if (selectedTheme == "Computo") {
			colorBg = TQColor(28, 46, 36);
			colorFg = TQColor(230, 242, 235);
			colorSmallFg = TQColor(30, 137, 11);
			colorNumKeyFg = TQColor(230, 242, 235);
			colorNumKeyBg = TQColor(42, 66, 53);
			colorOpKeyFg = TQColor(230, 242, 235);
			colorOpKeyBg = TQColor(33, 54, 42);
			colorClearKeyFg = TQColor(230, 242, 235);
			colorClearKeyBg = TQColor(33, 54, 42);
			colorMemKeyFg = TQColor(230, 242, 235);
			colorMemKeyBg = TQColor(33, 54, 42);
			colorEqualBg = TQColor(46, 139, 87);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 5;
			keyFontType = 0;
			keyFont = TQFont("Segoe UI", 12);
			colorDisplayFg = TQColor(57, 255, 20);
			colorDisplayBg = TQColor(11, 19, 14);
			colorPanelBg = TQColor(42, 66, 53);
			colorMenuBg = TQColor(42, 66, 53);
			noDeco = true;
			standardMenuBar = false;
			invertIcons = true;
			keysBorders = false;
			keysBordersColor = TQColor(0, 0, 0);
			windowBorder = false;
			windowBorderColor = TQColor(0, 0, 0);
			displayBorder = false;
			displayBorderColor = TQColor(0, 0, 0);
			boldIcons = false;
		} else if (selectedTheme == "Default" || selectedTheme == "internal_classic" || selectedTheme == "Classic") {
			colorBg = TQColor(242, 242, 242);
			colorFg = TQColor(0, 0, 0);
			colorSmallFg = TQColor(71, 71, 71);
			colorNumKeyFg = TQColor(0, 0, 0);
			colorNumKeyBg = TQColor(255, 255, 255);
			colorOpKeyFg = TQColor(0, 0, 0);
			colorOpKeyBg = TQColor(242, 242, 242);
			colorClearKeyFg = TQColor(0, 0, 0);
			colorClearKeyBg = TQColor(242, 242, 242);
			colorMemKeyFg = TQColor(0, 0, 0);
			colorMemKeyBg = TQColor(242, 242, 242);
			colorEqualBg = TQColor(0, 103, 192);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 0;
			keyFontType = 0;
			colorDisplayFg = TQColor(0, 0, 0);
			colorDisplayBg = TQColor(242, 242, 242);
			colorPanelBg = TQColor(231, 231, 231);
			colorMenuBg = TQColor(230, 230, 230);
			noDeco = false;
			standardMenuBar = false;
			invertIcons = false;
			keysBorders = false;
			keysBordersColor = TQColor(0, 0, 0);
			windowBorder = false;
			windowBorderColor = TQColor(0, 0, 0);
			displayBorder = false;
			displayBorderColor = TQColor(0, 0, 0);
			boldIcons = false;
		} else if (selectedTheme == "internal_classic_dark" || selectedTheme == "Classic Dark") {
			colorBg = TQColor(0, 0, 0);
			colorFg = TQColor(255, 255, 255);
			colorSmallFg = TQColor(222, 222, 222);
			colorNumKeyFg = TQColor(255, 255, 255);
			colorNumKeyBg = TQColor(0, 0, 0);
			colorOpKeyFg = TQColor(255, 255, 255);
			colorOpKeyBg = TQColor(0, 0, 0);
			colorClearKeyFg = TQColor(255, 255, 255);
			colorClearKeyBg = TQColor(0, 0, 0);
			colorMemKeyFg = TQColor(255, 255, 255);
			colorMemKeyBg = TQColor(0, 0, 0);
			colorEqualBg = TQColor(0, 103, 192);
			colorEqualFg = TQColor(255, 255, 255);
			displayType = 0;
			keyFontType = 0;
			colorDisplayFg = TQColor(255, 255, 255);
			colorDisplayBg = TQColor(0, 0, 0);
			colorPanelBg = TQColor(62, 62, 62);
			colorMenuBg = TQColor(44, 44, 44);
			noDeco = false;
			standardMenuBar = false;
			invertIcons = true;
			keysBorders = false;
			keysBordersColor = TQColor(0, 0, 0);
			windowBorder = false;
			windowBorderColor = TQColor(0, 0, 0);
			displayBorder = false;
			displayBorderColor = TQColor(0, 0, 0);
			boldIcons = false;
		} else {
			TDEConfig *themeConfig = new TDEConfig("calcrc");
			themeConfig->reparseConfiguration();
			themeConfig->setGroup("Theme_" + selectedTheme);
			colorBg = themeConfig->readColorEntry("BgColor", &defaultBg);
			colorFg = themeConfig->readColorEntry("FgColor", &defaultFg);
			colorSmallFg = themeConfig->readColorEntry("SmallFgColor", &defaultSmallFg);
			colorNumKeyFg = themeConfig->readColorEntry("NumKeyFgColor", &defaultNumKeyFg);
			colorNumKeyBg = themeConfig->readColorEntry("NumKeyBgColor", &defaultNumKeyBg);
			colorOpKeyFg = themeConfig->readColorEntry("OpKeyFgColor", &defaultOtherKeyFg);
			colorOpKeyBg = themeConfig->readColorEntry("OpKeyBgColor", &defaultOtherKeyBg);
			colorClearKeyFg = themeConfig->readColorEntry("ClearKeyFgColor", &defaultOtherKeyFg);
			colorClearKeyBg = themeConfig->readColorEntry("ClearKeyBgColor", &defaultOtherKeyBg);
			colorMemKeyFg = themeConfig->readColorEntry("MemKeyFgColor", &defaultOtherKeyFg);
			colorMemKeyBg = themeConfig->readColorEntry("MemKeyBgColor", &defaultOtherKeyBg);
			colorEqualBg = themeConfig->readColorEntry("EqualBgColor", &defaultEqualBg);
			colorEqualFg = themeConfig->readColorEntry("EqualFgColor", &defaultEqualFg);
			displayType = themeConfig->readNumEntry("DisplayType", 1);
			if (displayType == 2 && !themeConfig->hasKey("DisplayFont")) {
				displayType = 3; // LCD compatibility
			}
			displayFont = themeConfig->readFontEntry("DisplayFont", &defaultF);

			keyFontType = themeConfig->readNumEntry("KeyFontType", 1);
			TQFont defaultKF = font();
			keyFont = themeConfig->readFontEntry("KeyFont", &defaultKF);
			colorDisplayFg = themeConfig->readColorEntry("DisplayFgColor", &defaultDisplayFg);
			colorDisplayBg = themeConfig->readColorEntry("DisplayBgColor", &defaultDisplayBg);
			colorPanelBg = themeConfig->readColorEntry("PanelBgColor", &defaultPanelBg);
			colorMenuBg = themeConfig->readColorEntry("MenuBgColor", &colorPanelBg);
			noDeco = themeConfig->readBoolEntry("NoDeco", false);
			standardMenuBar = themeConfig->readBoolEntry("StandardMenuBar", false);
			invertIcons = themeConfig->readBoolEntry("InvertIcons", false);
			keysBorders = themeConfig->readBoolEntry("KeysBorders", false);
			keysBordersColor = themeConfig->readColorEntry("KeysBordersColor", &defaultKeysBorders);
			windowBorder = themeConfig->readBoolEntry("WindowBorder", false);
			windowBorderColor = themeConfig->readColorEntry("WindowBorderColor", &defaultWindowBorder);
			displayBorder = themeConfig->readBoolEntry("DisplayBorder", false);
			displayBorderColor = themeConfig->readColorEntry("DisplayBorderColor", &defaultDisplayBorder);
			boldIcons = themeConfig->readBoolEntry("BoldIcons", false);
			delete themeConfig;
		}
	} else { // Custom
		colorBg = config->readColorEntry("CustomBgColor", &defaultBg);
		colorFg = config->readColorEntry("CustomFgColor", &defaultFg);
		colorSmallFg = config->readColorEntry("CustomSmallFgColor", &defaultSmallFg);
		colorNumKeyFg = config->readColorEntry("CustomNumKeyFgColor", &defaultNumKeyFg);
		colorNumKeyBg = config->readColorEntry("CustomNumKeyBgColor", &defaultNumKeyBg);
		colorOpKeyFg = config->readColorEntry("CustomOpKeyFgColor", &defaultOtherKeyFg);
		colorOpKeyBg = config->readColorEntry("CustomOpKeyBgColor", &defaultOtherKeyBg);
		colorClearKeyFg = config->readColorEntry("CustomClearKeyFgColor", &defaultOtherKeyFg);
		colorClearKeyBg = config->readColorEntry("CustomClearKeyBgColor", &defaultOtherKeyBg);
		colorMemKeyFg = config->readColorEntry("CustomMemKeyFgColor", &defaultOtherKeyFg);
		colorMemKeyBg = config->readColorEntry("CustomMemKeyBgColor", &defaultOtherKeyBg);
		colorEqualBg = config->readColorEntry("CustomEqualBgColor", &defaultEqualBg);
		colorEqualFg = config->readColorEntry("CustomEqualFgColor", &defaultEqualFg);
		displayType = config->readNumEntry("CustomDisplayType", 0);
		if (displayType == 2 && !config->hasKey("CustomDisplayFont")) {
			displayType = 3; // LCD compatibility
		}
		displayFont = config->readFontEntry("CustomDisplayFont", &defaultF);

		keyFontType = config->readNumEntry("CustomKeyFontType", 0);
		TQFont defaultKF = font();
		keyFont = config->readFontEntry("CustomKeyFont", &defaultKF);
		colorDisplayFg = config->readColorEntry("CustomDisplayFgColor", &defaultDisplayFg);
		colorDisplayBg = config->readColorEntry("CustomDisplayBgColor", &defaultDisplayBg);
		colorPanelBg = config->readColorEntry("CustomPanelBgColor", &defaultPanelBg);
		colorMenuBg = config->readColorEntry("CustomMenuBgColor", &colorPanelBg);
		noDeco = config->readBoolEntry("CustomNoDeco", false);
		standardMenuBar = config->readBoolEntry("CustomStandardMenuBar", false);
		invertIcons = config->readBoolEntry("CustomInvertIcons", false);
		keysBorders = config->readBoolEntry("CustomKeysBorders", false);
		keysBordersColor = config->readColorEntry("CustomKeysBordersColor", &defaultKeysBorders);
		windowBorder = config->readBoolEntry("CustomWindowBorder", false);
		windowBorderColor = config->readColorEntry("CustomWindowBorderColor", &defaultWindowBorder);
		displayBorder = config->readBoolEntry("CustomDisplayBorder", false);
		displayBorderColor = config->readColorEntry("CustomDisplayBorderColor", &defaultDisplayBorder);
		boldIcons = config->readBoolEntry("CustomBoldIcons", false);
	}
	delete config;

	TQPalette winPal = palette();
	winPal.setColor(TQPalette::Active, TQColorGroup::Background, colorBg);
	winPal.setColor(TQPalette::Inactive, TQColorGroup::Background, colorBg);
	winPal.setColor(TQPalette::Active, TQColorGroup::Foreground, colorFg);
	winPal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, colorFg);
	setPalette(winPal);
	setPaletteBackgroundColor(colorBg);

	if (centralWidget()) {
		centralWidget()->setPalette(winPal);
		centralWidget()->setPaletteBackgroundColor(colorBg);
	}

	TQPalette panelPal = winPal;
	panelPal.setColor(TQPalette::Active, TQColorGroup::Background, colorPanelBg);
	panelPal.setColor(TQPalette::Inactive, TQColorGroup::Background, colorPanelBg);

	if (mHeaderWidget) {
		mHeaderWidget->setPalette(winPal);
		mHeaderWidget->setPaletteBackgroundColor(colorBg);
	}
	if (BaseChooseGroup) {
		BaseChooseGroup->setPalette(winPal);
		BaseChooseGroup->setPaletteBackgroundColor(colorBg);
	}
	if (mProgrammerPage) {
		mProgrammerPage->setPalette(winPal);
		mProgrammerPage->setPaletteBackgroundColor(colorBg);
	}
	if (mBitboardPage) {
		mBitboardPage->setPalette(winPal);
		mBitboardPage->setPaletteBackgroundColor(colorBg);
	}
	if (mDateCalcPage) {
		mDateCalcPage->setPalette(winPal);
		mDateCalcPage->setPaletteBackgroundColor(colorBg);
	}
	if (mConverterPage) {
		mConverterPage->setPalette(winPal);
		mConverterPage->setPaletteBackgroundColor(colorBg);
		if (m_converterTopPanel) {
			m_converterTopPanel->setPalette(winPal);
			m_converterTopPanel->setPaletteBackgroundColor(colorBg);
		}
		if (m_lblEquiv) {
			m_lblEquiv->setPalette(winPal);
			m_lblEquiv->setPaletteBackgroundColor(colorBg);
			m_lblEquiv->setPaletteForegroundColor(colorFg);
		}
		if (m_lblEquivTitle) {
			m_lblEquivTitle->setPalette(winPal);
			m_lblEquivTitle->setPaletteBackgroundColor(colorBg);
		}
		if (m_comboUnit1) {
			m_comboUnit1->setPalette(winPal);
		}
		if (m_comboUnit2) {
			m_comboUnit2->setPalette(winPal);
		}
	}
	if (mScientificPage) {
		mScientificPage->setPalette(winPal);
		mScientificPage->setPaletteBackgroundColor(colorBg);
	}
	if (mStandardPage) {
		mStandardPage->setPalette(winPal);
		mStandardPage->setPaletteBackgroundColor(colorBg);
	}



	TQPalette menuPal = winPal;
	menuPal.setColor(TQPalette::Active, TQColorGroup::Background, colorMenuBg);
	menuPal.setColor(TQPalette::Inactive, TQColorGroup::Background, colorMenuBg);
	menuPal.setColor(TQPalette::Active, TQColorGroup::Button, colorMenuBg);
	menuPal.setColor(TQPalette::Inactive, TQColorGroup::Button, colorMenuBg);
	menuPal.setColor(TQPalette::Active, TQColorGroup::Foreground, colorFg);
	menuPal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, colorFg);
	menuPal.setColor(TQPalette::Active, TQColorGroup::ButtonText, colorFg);
	menuPal.setColor(TQPalette::Inactive, TQColorGroup::ButtonText, colorFg);
	
	int valMenu = (colorMenuBg.red() * 299 + colorMenuBg.green() * 587 + colorMenuBg.blue() * 114) / 1000;
	TQColor highlightColor = (valMenu > 128) ? colorMenuBg.dark(112) : colorMenuBg.light(125);
	if (valMenu < 10) highlightColor = TQColor(50, 50, 50);
	TQColor highlightText = (valMenu > 128) ? TQColor(0, 0, 0) : TQColor(255, 255, 255);
	
	menuPal.setColor(TQPalette::Active, TQColorGroup::Highlight, highlightColor);
	menuPal.setColor(TQPalette::Inactive, TQColorGroup::Highlight, highlightColor);
	menuPal.setColor(TQPalette::Active, TQColorGroup::HighlightedText, highlightText);
	menuPal.setColor(TQPalette::Inactive, TQColorGroup::HighlightedText, highlightText);

	if (menuBar()) {
		if (standardMenuBar) {
			menuBar()->show();
			if (pbHamburger) pbHamburger->hide();
		} else {
			menuBar()->hide();
			if (pbHamburger) pbHamburger->show();
		}
		menuBar()->setPalette(menuPal);
		menuBar()->setPaletteBackgroundColor(colorMenuBg);
	}

	TQObjectList *allPopups = queryList("TQPopupMenu");
	if (allPopups) {
		TQObjectListIt it(*allPopups);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			TQPopupMenu *m = (TQPopupMenu*)obj;
			m->setPalette(menuPal);
		}
		delete allPopups;
	}

	bool wasVisible = isVisible();
	TQPoint oldPos = pos();
	WFlags flags = getWFlags();

	bool needsBorderChange = (noDeco != m_noDeco);

	if (needsBorderChange) {
		m_noDeco = noDeco;
		if (wasVisible) {
			hide();
			if (noDeco) {
				reparent(NULL, flags | WStyle_Customize | WStyle_NoBorder, oldPos, false);
			} else {
				reparent(NULL, flags & ~(WStyle_Customize | WStyle_NoBorder), oldPos, false);
			}
			move(oldPos);
			show();
			move(oldPos);
		} else {
			if (noDeco) {
				setWFlags(flags | WStyle_Customize | WStyle_NoBorder);
			} else {
				setWFlags(flags & ~(WStyle_Customize | WStyle_NoBorder));
			}
		}



		if (noDeco) {
			if (pbMinimize) pbMinimize->show();
			if (pbMaximize) {
				if (isMaximized()) {
					applyIconToButton(pbMaximize, unmaximize_png, unmaximize_png_len, TQString("Restore"));
				} else {
					applyIconToButton(pbMaximize, maximize_png, maximize_png_len, TQString("Maximize"));
				}
				pbMaximize->show();
			}
			if (pbClose) pbClose->show();
		} else {
			if (pbMinimize) pbMinimize->hide();
			if (pbMaximize) pbMaximize->hide();
			if (pbClose) pbClose->hide();
		}
	}

	// Invalidate icon cache and update all buttons containing custom icons
	IconUtils::clearCache();
	updateConversionMimeIcons();
	CalcButton::keys_borders = keysBorders;
	CalcButton::keys_borders_color = keysBordersColor;
	TQObjectList *btns = queryList("CalcButton");
	if (btns) {
		TQObjectListIt it(*btns);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			CalcButton *btn = (CalcButton*)obj;
			btn->setPalette(getButtonPalette(btn, winPal,
			                                 colorNumKeyFg, colorNumKeyBg,
			                                 colorOpKeyFg, colorOpKeyBg,
			                                 colorClearKeyFg, colorClearKeyBg,
			                                 colorMemKeyFg, colorMemKeyBg,
			                                 colorEqualBg, colorEqualFg,
			                                 colorPanelBg, colorFg, colorBg));
			if (btn->iconData() && btn->iconLen() > 0) {
				btn->invalidateIconCache();
			} else {
				if (keyFontType == 0) {
					btn->setFont(TQFont("Segoe Calc"));
				} else if (keyFontType == 2) {
					btn->setFont(keyFont);
				} else {
					btn->setFont(TQFont());
				}
			}
			btn->update();
		}
		delete btns;
	}

	m_displayType = displayType;
	m_displayFont = displayFont;
	m_colorDisplayBg = colorDisplayBg;
	m_colorDisplayFg = colorDisplayFg;
	m_invertIcons = invertIcons;
	m_colorPanelBg = colorPanelBg;
	m_colorPanelFg = colorFg;
	m_colorMenuBg = colorMenuBg;

	if (mProgrammerMemoryFrame) {
		mProgrammerMemoryFrame->setPaletteBackgroundColor(colorPanelBg);
	}
	if (mProgMemoryFrameScroll) {
		mProgMemoryFrameScroll->setPaletteBackgroundColor(colorPanelBg);
		mProgMemoryFrameScroll->viewport()->setPaletteBackgroundColor(colorPanelBg);
	}
	if (mProgMemoryContainer) {
		mProgMemoryContainer->setPaletteBackgroundColor(colorPanelBg);
		const TQObjectList *childList = mProgMemoryContainer->children();
		if (childList) {
			TQObjectListIt it(*childList);
			TQObject *obj;
			while ((obj = it.current()) != 0) {
				++it;
				ProgMemoryRowWidget *row = dynamic_cast<ProgMemoryRowWidget*>(obj);
				if (row) {
					row->updateColors();
				}
			}
		}
	}
	if (mProgMemoryDeleteButton) {
		mProgMemoryDeleteButton->setPaletteBackgroundColor(colorPanelBg);
		mProgMemoryDeleteButton->setBackgroundColor(colorPanelBg);
	}
	if (mProgMemoryBottomBar) {
		mProgMemoryBottomBar->setPaletteBackgroundColor(colorPanelBg);
	}

	if (m_lblVal1_lcd) {
		m_lblVal1_lcd->setColorBackground1(colorDisplayBg);
		m_lblVal1_lcd->setColorBackground2(colorDisplayBg);
		m_lblVal1_lcd->setColorPixel(colorDisplayFg);
	}
	if (m_lblVal2_lcd) {
		m_lblVal2_lcd->setColorBackground1(colorDisplayBg);
		m_lblVal2_lcd->setColorBackground2(colorDisplayBg);
		m_lblVal2_lcd->setColorPixel(colorDisplayFg);
	}
	if (mConverterPage && mConverterPage->isVisible()) {
		updateConverterSizes();
	}

	if (calc_display) {
		calc_display->setDisplayType(displayType);
		calc_display->setCustomDisplayFont(displayFont);
		TQPalette dispPal = calc_display->palette();
		dispPal.setColor(TQPalette::Active, TQColorGroup::Background, colorDisplayBg);
		dispPal.setColor(TQPalette::Inactive, TQColorGroup::Background, colorDisplayBg);
		dispPal.setColor(TQPalette::Active, TQColorGroup::Foreground, colorDisplayFg);
		dispPal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, colorDisplayFg);
		dispPal.setColor(TQPalette::Active, TQColorGroup::Text, colorDisplayFg);
		dispPal.setColor(TQPalette::Inactive, TQColorGroup::Text, colorDisplayFg);
		calc_display->setPalette(dispPal);
		calc_display->setPaletteBackgroundColor(colorDisplayBg);
		calc_display->setSmallTextForeground(colorSmallFg);
		calc_display->setDisplayBorder(displayBorder, displayBorderColor);
	}

	m_windowBorder = windowBorder;
	m_windowBorderColor = windowBorderColor;
	if (m_historyWindow) {
		m_historyWindow->setColors(colorPanelBg, colorDisplayFg, colorDisplayBg);
	}

	if (centralWidget()) {
		centralWidget()->update();
	}
}

void Calculator::resizeEvent(TQResizeEvent *e)
{
	TQMainWindow::resizeEvent(e);

	if (_calc_mode == ModeDateCalc) {
		updateDateCalcPageSizes();
	} else if (_calc_mode >= ModeVolume) {
		updateConverterSizes();
	}
	updateBitboardLabelSizes();
	updateProgMemorySizes();
}

void Calculator::showEvent(TQShowEvent *e)
{
	TQMainWindow::showEvent(e);

#if defined(TQ_WS_X11)
	Display *dpy = x11Display();
	Window win = winId();
	if (dpy && win) {
		MWMHints hints;
		hints.flags = MWM_HINTS_DECORATIONS;
		hints.decorations = m_noDeco ? MWM_DECOR_NONE : MWM_DECOR_ALL;
		hints.functions = 0;
		hints.input_mode = 0;
		hints.status = 0;
		Atom motif_wm_hints = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
		XChangeProperty(dpy, win, motif_wm_hints, motif_wm_hints, 32, PropModeReplace, (unsigned char *)&hints, 5);
		XFlush(dpy);
	}
#endif
}

void Calculator::updateDateCalcPageSizes()
{
	if (!mDateCalcPage || !mDateModeCombo) return;

	int h = height();
	if (h < 50) return;

	int baseSize = h * 0.028;
	if (baseSize < 12) baseSize = 12;

	TQFont f = mDateModeCombo->font();
	f.setPixelSize(baseSize + 2);
	mDateModeCombo->setFont(f);

	f.setPixelSize(baseSize);
	mDateFromPicker->setFont(f);
	mDateToPicker->setFont(f);
	mDateDiffLabel->setFont(f);

	// Calculate exact proportion used in other views for icons
	int btnSize = h * 0.056;
	if (btnSize < 16) btnSize = 16;
	mDateFromPicker->setButtonSize(btnSize);
	mDateToPicker->setButtonSize(btnSize);
	
	TQFont fDays = f;
	fDays.setItalic(true);
	mDateDiffResultDays->setFont(fDays);

	TQFont fResult = f;
	fResult.setBold(true);
	fResult.setPixelSize(baseSize + 6);
	mDateDiffResult->setFont(fResult);

	if (mAddRadio) mAddRadio->setFont(f);
	if (mSubRadio) mSubRadio->setFont(f);
	if (mYearsCombo) mYearsCombo->setFont(f);
	if (mMonthsCombo) mMonthsCombo->setFont(f);
	if (mDaysCombo) mDaysCombo->setFont(f);

	if (mDateCalcPage->layout()) {
		int margin = h * 0.035;
		int spacing = h * 0.021;
		if (margin > 40) margin = 40;
		if (spacing > 30) spacing = 30;
		mDateCalcPage->layout()->setMargin(margin);
		mDateCalcPage->layout()->setSpacing(spacing);
	}

	// we also need to update the labels "From" and "To". 
	// Since we didn't save their pointers, we can iterate through the children of mDateCalcPage
	TQObjectList *list = mDateCalcPage->queryList("TQLabel");
	if (list) {
		TQObjectListIt it(*list);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			TQLabel *lbl = (TQLabel*)obj;
			if (lbl != mDateDiffResult && lbl != mDateDiffResultDays && lbl != mDateDiffLabel) {
				lbl->setFont(f);
			}
		}
		delete list;
	}
}

void Calculator::set_precision()
{
	KNumber:: setDefaultFloatPrecision(9);
	UpdateDisplay(false);
}


bool Calculator::handleKeyPress(TQKeyEvent *e)
{
	int keyVal = e->key();
	TQString text = e->text();
	int state = e->state();

	// Check for Ctrl+C and Ctrl+V first
	if ((state & ControlButton) && !(state & AltButton) && !(state & ShiftButton)) {
		if (keyVal == Key_C) {
			if (_calc_mode >= ModeVolume) {
				TQString txt = (m_convActiveSlot == 1) ? m_lblVal1->text() : m_lblVal2->text();
				(TQApplication::clipboard())->setText(txt, TQClipboard::Clipboard);
				(TQApplication::clipboard())->setText(txt, TQClipboard::Selection);
			} else if (_calc_mode == ModeDateCalc) {
				if (mDateDiffResult) {
					TQString txt = mDateDiffResult->text();
					if (mDateDiffResultDays && !mDateDiffResultDays->text().isEmpty()) {
						txt += " (" + mDateDiffResultDays->text() + ")";
					}
					(TQApplication::clipboard())->setText(txt, TQClipboard::Clipboard);
					(TQApplication::clipboard())->setText(txt, TQClipboard::Selection);
				}
			} else if (calc_display) {
				calc_display->slotCopy();
			}
			return true;
		}
		else if (keyVal == Key_V) {
			if (_calc_mode >= ModeVolume) {
				TQString tmp_str = (TQApplication::clipboard())->text(TQClipboard::Clipboard).stripWhiteSpace();
				if (!tmp_str.isNull()) {
					tmp_str.replace('.', ',');
					if (m_convActiveSlot == 1) {
						m_convInput1 = tmp_str;
						setVal1Text(tmp_str);
					} else {
						m_convInput2 = tmp_str;
						setVal2Text(tmp_str);
					}
					recalculateConversion();
				}
			} else if (calc_display) {
				calc_display->slotPaste(true);
			}
			return true;
		}
	}

	// Do not intercept if Ctrl or Alt is pressed
	if (state & (ControlButton | AltButton)) {
		return false;
	}

	// 1. Digits 0-9
	int digit = -1;
	if (text.length() == 1) {
		char ch = text.at(0).latin1();
		if (ch >= '0' && ch <= '9') {
			digit = ch - '0';
		}
	}
	if (digit == -1 && keyVal >= Key_0 && keyVal <= Key_9) {
		digit = keyVal - Key_0;
	}

	if (digit >= 0 && digit <= 9) {
		if (_calc_mode >= ModeVolume) {
			handleConvDigit(digit);
			return true;
		} else if (_calc_mode == ModeStandard) {
			if (StdNumButtonGroup && StdNumButtonGroup->find(digit) && StdNumButtonGroup->find(digit)->isEnabled()) {
				StdNumButtonGroup->find(digit)->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (SciNumButtonGroup && SciNumButtonGroup->find(digit) && SciNumButtonGroup->find(digit)->isEnabled()) {
				SciNumButtonGroup->find(digit)->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (ProgNumButtonGroup && ProgNumButtonGroup->find(digit) && ProgNumButtonGroup->find(digit)->isEnabled()) {
				ProgNumButtonGroup->find(digit)->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeStatistics) {
			if (NumButtonGroup && NumButtonGroup->find(digit) && NumButtonGroup->find(digit)->isEnabled()) {
				NumButtonGroup->find(digit)->animateClick();
				return true;
			}
		}
	}

	// 1.5 Hex letters A-F in Programmer mode
	int hexVal = -1;
	if (text.length() == 1) {
		char ch = text.at(0).upper().latin1();
		if (ch >= 'A' && ch <= 'F') {
			hexVal = 10 + (ch - 'A');
		}
	}
	if (hexVal == -1 && keyVal >= Key_A && keyVal <= Key_F) {
		hexVal = 10 + (keyVal - Key_A);
	}

	if (hexVal >= 10 && hexVal <= 15) {
		if (_calc_mode == ModeProgrammer) {
			if (ProgNumButtonGroup && ProgNumButtonGroup->find(hexVal) && ProgNumButtonGroup->find(hexVal)->isEnabled()) {
				ProgNumButtonGroup->find(hexVal)->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeStatistics) {
			if (NumButtonGroup && NumButtonGroup->find(hexVal) && NumButtonGroup->find(hexVal)->isEnabled()) {
				NumButtonGroup->find(hexVal)->animateClick();
				return true;
			}
		}
	}

	// 2. Parentheses
	if (text == "(" || keyVal == Key_ParenLeft) {
		if (_calc_mode == ModeScientific) {
			if (pbSciParenL && pbSciParenL->isEnabled()) {
				pbSciParenL->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgParenL && pbProgParenL->isEnabled()) {
				pbProgParenL->animateClick();
				return true;
			}
		} else if (_calc_mode != ModeStandard && _calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbParenOpen && pbParenOpen->isEnabled()) {
				pbParenOpen->animateClick();
				return true;
			}
		}
	} else if (text == ")" || keyVal == Key_ParenRight) {
		if (_calc_mode == ModeScientific) {
			if (pbSciParenR && pbSciParenR->isEnabled()) {
				pbSciParenR->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgParenR && pbProgParenR->isEnabled()) {
				pbProgParenR->animateClick();
				return true;
			}
		} else if (_calc_mode != ModeStandard && _calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbParenClose && pbParenClose->isEnabled()) {
				pbParenClose->animateClick();
				return true;
			}
		}
	}

	// 3. Decimal point
	if (text == "." || text == "," || keyVal == Key_Period || keyVal == Key_Comma) {
		if (_calc_mode >= ModeVolume) {
			handleConvComma();
			return true;
		} else if (_calc_mode == ModeStandard) {
			if (pbStdPeriod && pbStdPeriod->isEnabled()) {
				pbStdPeriod->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciDot && pbSciDot->isEnabled()) {
				pbSciDot->animateClick();
				return true;
			}
		} else if (_calc_mode != ModeProgrammer && _calc_mode != ModeDateCalc) {
			if (pbPeriod && pbPeriod->isEnabled()) {
				pbPeriod->animateClick();
				return true;
			}
		}
	}

	// 4. Operators
	if (text == "+" || keyVal == Key_Plus) {
		if (_calc_mode == ModeStandard) {
			if (pbStdPlus && pbStdPlus->isEnabled()) {
				pbStdPlus->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciAdd && pbSciAdd->isEnabled()) {
				pbSciAdd->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgAdd && pbProgAdd->isEnabled()) {
				pbProgAdd->animateClick();
				return true;
			}
		} else if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbPlus && pbPlus->isEnabled()) {
				pbPlus->animateClick();
				return true;
			}
		}
	} else if (text == "-" || keyVal == Key_Minus) {
		if (_calc_mode == ModeStandard) {
			if (pbStdMinus && pbStdMinus->isEnabled()) {
				pbStdMinus->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciSub && pbSciSub->isEnabled()) {
				pbSciSub->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgSub && pbProgSub->isEnabled()) {
				pbProgSub->animateClick();
				return true;
			}
		} else if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbMinus && pbMinus->isEnabled()) {
				pbMinus->animateClick();
				return true;
			}
		}
	} else if (text == "*" || text == "x" || text == "\xc3\x97" || keyVal == Key_Asterisk || keyVal == Key_multiply) {
		if (_calc_mode == ModeStandard) {
			if (pbStdX && pbStdX->isEnabled()) {
				pbStdX->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciMul && pbSciMul->isEnabled()) {
				pbSciMul->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgMul && pbProgMul->isEnabled()) {
				pbProgMul->animateClick();
				return true;
			}
		} else if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbX && pbX->isEnabled()) {
				pbX->animateClick();
				return true;
			}
		}
	} else if (text == "/" || keyVal == Key_Slash || keyVal == Key_division) {
		if (_calc_mode == ModeStandard) {
			if (pbStdDivision && pbStdDivision->isEnabled()) {
				pbStdDivision->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciDiv && pbSciDiv->isEnabled()) {
				pbSciDiv->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgDiv && pbProgDiv->isEnabled()) {
				pbProgDiv->animateClick();
				return true;
			}
		} else if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbDivision && pbDivision->isEnabled()) {
				pbDivision->animateClick();
				return true;
			}
		}
	}

	// 5. Equals / Result
	if (text == "=" || keyVal == Key_Equal || keyVal == Key_Enter || keyVal == Key_Return) {
		if (_calc_mode == ModeStandard) {
			if (pbStdEqual && pbStdEqual->isEnabled()) {
				pbStdEqual->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciEq && pbSciEq->isEnabled()) {
				pbSciEq->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgEq && pbProgEq->isEnabled()) {
				pbProgEq->animateClick();
				return true;
			}
		} else if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbEqual && pbEqual->isEnabled()) {
				pbEqual->animateClick();
				return true;
			}
		}
	}

	// 6. Backspace
	if (keyVal == Key_Backspace) {
		if (_calc_mode >= ModeVolume) {
			handleConvBS();
			return true;
		} else if (_calc_mode == ModeStandard) {
			if (pbStdBackSpace && pbStdBackSpace->isEnabled()) {
				pbStdBackSpace->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciBS && pbSciBS->isEnabled()) {
				pbSciBS->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgBS && pbProgBS->isEnabled()) {
				pbProgBS->animateClick();
				return true;
			}
		} else if (_calc_mode != ModeDateCalc) {
			calc_display->deleteLastDigit();
			return true;
		}
	}

	// 7. Clear Entry (CE)
	if (keyVal == Key_Delete) {
		if (_calc_mode >= ModeVolume) {
			handleConvCE();
			return true;
		} else if (_calc_mode == ModeStandard) {
			if (pbStdCE && pbStdCE->isEnabled()) {
				pbStdCE->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciCE && pbSciCE->isEnabled()) {
				pbSciCE->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgCE && pbProgCE->isEnabled()) {
				pbProgCE->animateClick();
				return true;
			}
		} else if (_calc_mode != ModeDateCalc) {
			if (pbClear && pbClear->isEnabled()) {
				pbClear->animateClick();
				return true;
			}
		}
	}

	// 8. Clear (C) (using Prior / Page Up)
	if (keyVal == Key_Prior) {
		if (_calc_mode == ModeStandard) {
			if (pbStdC && pbStdC->isEnabled()) {
				pbStdC->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeScientific) {
			if (pbSciC && pbSciC->isEnabled()) {
				pbSciC->animateClick();
				return true;
			}
		} else if (_calc_mode == ModeProgrammer) {
			if (pbProgC && pbProgC->isEnabled()) {
				pbProgC->animateClick();
				return true;
			}
		} else if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbClear && pbClear->isEnabled()) {
				pbClear->animateClick();
				return true;
			}
		}
	}

	// 9. Clear All (AC) (using Next / Page Down)
	if (keyVal == Key_Next) {
		if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbAC && pbAC->isEnabled()) {
				pbAC->animateClick();
				return true;
			}
		}
	}

	// 10. Percent (%)
	if (text == "%" || keyVal == Key_Percent) {
		if (_calc_mode == ModeStandard) {
			if (pbStdPercent && pbStdPercent->isEnabled()) {
				pbStdPercent->animateClick();
				return true;
			}
		} else if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbPercent && pbPercent->isEnabled()) {
				pbPercent->animateClick();
				return true;
			}
		}
	}

	// 11. Square
	if (text == TQString::fromUtf8("²") || keyVal == Key_twosuperior) {
		if (_calc_mode < ModeVolume && _calc_mode != ModeDateCalc) {
			if (pbSquare && pbSquare->isEnabled()) {
				pbSquare->animateClick();
				return true;
			}
		}
	}

	// 12. Exponent (E) in Scientific Mode
	if (text.upper() == "E" || keyVal == Key_E) {
		if (_calc_mode == ModeScientific) {
			if (pbEE && pbEE->isEnabled()) {
				pbEE->animateClick();
				return true;
			}
		}
	}

	return false;
}

#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif

bool Calculator::eventFilter(TQObject *o, TQEvent *e)
{
	if (e->type() == TQEvent::KeyPress) {
		TQKeyEvent *ke = static_cast<TQKeyEvent*>(e);
		if (o->isWidgetType()) {
			TQWidget *w = static_cast<TQWidget*>(o);
			if (w->topLevelWidget() == this) {
				if (handleKeyPress(ke)) {
					return true; // Eat the event
				}
			}
		}
	}

	if (o == mHeaderWidget || o == lblModeTitle || o == calc_display) {
		if (e->type() == TQEvent::MouseButtonPress) {
			TQMouseEvent *mouseEvent = static_cast<TQMouseEvent*>(e);
			if (mouseEvent->button() == TQt::LeftButton) {
				m_isDragging = true;
				m_dragOffset = mouseEvent->globalPos() - this->pos();
				return false;
			}
		} else if (e->type() == TQEvent::MouseMove) {
			if (m_isDragging) {
				TQMouseEvent *mouseEvent = static_cast<TQMouseEvent*>(e);
				this->move(mouseEvent->globalPos() - m_dragOffset);
				return true;
			}
		} else if (e->type() == TQEvent::MouseButtonRelease) {
			TQMouseEvent *mouseEvent = static_cast<TQMouseEvent*>(e);
			if (mouseEvent->button() == TQt::LeftButton && m_isDragging) {
				m_isDragging = false;
				return false;
			}
		}
	}

	if (_calc_mode >= ModeVolume && e->type() == TQEvent::MouseButtonPress) {
		TQMouseEvent *mouseEvent = static_cast<TQMouseEvent*>(e);
		if (mouseEvent->button() == TQt::RightButton) {
			if (o == m_lblVal1 || o == m_lblVal2 || o == m_lblVal1_lcd || o == m_lblVal2_lcd) {
				TQPopupMenu *menu = new TQPopupMenu(this);
				TQPixmap pixCopy = IconUtils::load(copy_png, copy_png_len, 32, 32);
				TQPixmap pixPaste = IconUtils::load(paste_png, paste_png_len, 32, 32);
				menu->insertItem(pixCopy, tr_str("Copy"), 1);
				menu->insertItem(pixPaste, tr_str("Paste"), 2);
				int selected = menu->exec(mouseEvent->globalPos());
				delete menu;
				
				if (selected == 1) {
					TQString txt = (o == m_lblVal1 || o == m_lblVal1_lcd) ? m_lblVal1->text() : m_lblVal2->text();
					(TQApplication::clipboard())->setText(txt, TQClipboard::Clipboard);
					(TQApplication::clipboard())->setText(txt, TQClipboard::Selection);
				} else if (selected == 2) {
					TQString tmp_str = (TQApplication::clipboard())->text(TQClipboard::Clipboard).stripWhiteSpace();
					if (!tmp_str.isNull()) {
						tmp_str.replace('.', ',');
						if (o == m_lblVal1 || o == m_lblVal1_lcd) {
							m_convInput1 = tmp_str;
							setVal1Text(tmp_str);
							setActiveSlot(1);
						} else {
							m_convInput2 = tmp_str;
							setVal2Text(tmp_str);
							setActiveSlot(2);
						}
						recalculateConversion();
					}
				}
				return true;
			}
		} else {
			if (o == m_lblVal1 || o == m_lblVal1_lcd) {
				setActiveSlot(1);
				return true;
			} else if (o == m_lblVal2 || o == m_lblVal2_lcd) {
				setActiveSlot(2);
				return true;
			}
		}
	}

	if (_calc_mode == ModeDateCalc && e->type() == TQEvent::MouseButtonPress) {
		TQMouseEvent *mouseEvent = static_cast<TQMouseEvent*>(e);
		if (mouseEvent->button() == TQt::RightButton) {
			if (o == mDateDiffResult || o == mDateDiffResultDays) {
				TQLabel *lbl = static_cast<TQLabel*>(o);
				TQString txt = lbl->text();
				if (!txt.isEmpty()) {
					TQPopupMenu *menu = new TQPopupMenu(this);
					TQPixmap pixCopy = IconUtils::load(copy_png, copy_png_len, 32, 32);
					menu->insertItem(pixCopy, tr_str("Copy"), 1);
					int selected = menu->exec(mouseEvent->globalPos());
					delete menu;
					
					if (selected == 1) {
						(TQApplication::clipboard())->setText(txt, TQClipboard::Clipboard);
						(TQApplication::clipboard())->setText(txt, TQClipboard::Selection);
					}
				}
				return true;
			}
		}
	}

	if (mProgMemoryFrameScroll && o == mProgMemoryFrameScroll->viewport() && e->type() == TQEvent::Resize && mProgMemoryContainer) {
		mProgMemoryContainer->resize(mProgMemoryFrameScroll->visibleWidth(), mProgMemoryContainer->sizeHint().height());
		mProgMemoryFrameScroll->resizeContents(mProgMemoryContainer->width(), mProgMemoryContainer->height());
	}

	if(e->type() == TQEvent::DragEnter)
	{
		TQDragEnterEvent *ev = (TQDragEnterEvent *)e;
		//ev->accept(KColorDrag::canDecode(ev));
		return true;
	}
	else if(e->type() == TQEvent::DragLeave)
	{
		return true;
	}
	else if(e->type() == TQEvent::Drop)
	{
		if(!o->isA("CalcButton"))
			return false;

		TQColor c;
		TQDropEvent *ev = (TQDropEvent *)e;
		/*if( KColorDrag::decode(ev, c))
		{
		    // some logic here that I should comment out entirely... I'll just change the if to if(false)
		}*/
		if (false)
		{
		        TQPtrList<CalcButton> *list;
			int num_but;
			if((num_but = NumButtonGroup->id((CalcButton*)o))
			   != -1)
			{
			  TQPalette pal(c, palette().active().background());

			  // Was it hex-button or normal digit??
			  if (num_but <10)
			    for(int i=0; i<10; i++)
			      (NumButtonGroup->find(i))->setPalette(pal);
			  else
			    for(int i=10; i<16; i++)
			      (NumButtonGroup->find(i))->setPalette(pal);

			  return true;
			}
			else if( mFunctionButtonList.findRef((CalcButton*)o) != -1)
			{
				list = &mFunctionButtonList;
			}
			else if( mStatButtonList.findRef((CalcButton*)o) != -1)
			{
				list = &mStatButtonList;
			}
			else if( mMemButtonList.findRef((CalcButton*)o) != -1)
			{
				list = &mMemButtonList;
			}
			else if( mOperationButtonList.findRef((CalcButton*)o) != -1)
			{
				list = &mOperationButtonList;
			}
			else
				return false;

			TQPalette pal(c, palette().active().background());

			for(CalcButton *p = list->first(); p; p=list->next())
				p->setPalette(pal);
		}

		return true;
	}
	else
	{
		return TQMainWindow::eventFilter(o, e);
	}
}




////////////////////////////////////////////////////////////////
// Include the meta-object code for classes in this file
//

#include "calc.moc"


#include "tqtembeddedimages.h"
#include "embedded_fonts.h"
#include <fontconfig/fontconfig.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
#include <stdlib.h>

static std::vector<int> g_embeddedFontFds;
static std::vector<TQString> g_embeddedFontTempFiles;

static void loadEmbeddedFonts() {
    FcInit();
    struct FontData {
        const unsigned char* data;
        unsigned int len;
        const char* name;
    };

    FontData fonts[] = {
        { computo_ttf, computo_ttf_len, "Computo" },
        { digital_counter_7_ttf, digital_counter_7_ttf_len, "Digital Counter" },
        { segoecalc_ttf, segoecalc_ttf_len, "Segoe Calc" },
        { pocketcalculator_ttf, pocketcalculator_ttf_len, "Pocket Calculator" },
        { casio_ttf, casio_ttf_len, "ClassWiz Math CW" }
    };

    FcConfig* config = FcConfigGetCurrent();

    for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i) {
        // Try memfd_create first
        int fd = memfd_create(fonts[i].name, 0);
        if (fd >= 0) {
            ssize_t written = write(fd, fonts[i].data, fonts[i].len);
            if (written == static_cast<ssize_t>(fonts[i].len)) {
                TQString fdPath = TQString("/proc/self/fd/%1").arg(fd);
                if (FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(fdPath.latin1()))) {
                    g_embeddedFontFds.push_back(fd);
                    continue;
                }
            }
            close(fd);
        }

        // Fallback to /dev/shm or /tmp
        TQString tempPattern = "/dev/shm/kcalc_font_XXXXXX";
        char tempPath[256];
        tqstrncpy(tempPath, tempPattern.latin1(), sizeof(tempPath));
        int tempFd = mkstemp(tempPath);
        if (tempFd < 0) {
            tempPattern = "/tmp/kcalc_font_XXXXXX";
            tqstrncpy(tempPath, tempPattern.latin1(), sizeof(tempPath));
            tempFd = mkstemp(tempPath);
        }

        if (tempFd >= 0) {
            ssize_t written = write(tempFd, fonts[i].data, fonts[i].len);
            close(tempFd);
            if (written == static_cast<ssize_t>(fonts[i].len)) {
                if (FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(tempPath))) {
                    g_embeddedFontTempFiles.push_back(TQString(tempPath));
                    continue;
                }
            }
            unlink(tempPath);
        }
    }
}

static void cleanupEmbeddedFonts() {
    for (size_t i = 0; i < g_embeddedFontTempFiles.size(); ++i) {
        unlink(g_embeddedFontTempFiles[i].latin1());
    }
    for (size_t i = 0; i < g_embeddedFontFds.size(); ++i) {
        close(g_embeddedFontFds[i]);
    }
}

int main(int argc, char *argv[])
{
	tqt_embimg_init();

	TDEApplication app(argc, argv, TQCString("calc"));

	loadEmbeddedFonts();

	Calculator *calc = new Calculator;

	app.setMainWidget(calc);
	calc->setCaption(TQString("Calculator"));
	calc->resize(530, 590);
	calc->show();

	int exitCode = app.exec();

	cleanupEmbeddedFonts();

	return(exitCode);
}


TQWidget* Calculator::setupScientificKeys_win10(TQWidget *parent)
{
	mScientificPage = new TQWidget(parent);
	SciNumButtonGroup = new TQButtonGroup(0, "Sci-Num-Button-Group");
	connect(SciNumButtonGroup, TQ_SIGNAL(clicked(int)), TQ_SLOT(slotNumberclicked(int)));
	TQHBoxLayout *topLayout = new TQHBoxLayout(mScientificPage, 0, mInternalSpacing);


	pbSciAngle = new CalcButton("DEG", mScientificPage, "SciAngle-Button", TQString("Degrees"));
	pbSciAngle->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbSciAngle->setButtonType(CalcButton::TypeOther);
	topLayout->addWidget(pbSciAngle);
	connect(pbSciAngle, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotSciAngleClicked(void)));

	pbSciHyp = new CalcButton("HYP", mScientificPage, "SciHyp-Button", TQString("Hyperbolic"));
	pbSciHyp->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbSciHyp->setButtonType(CalcButton::TypeOther);
	pbSciHyp->setToggleButton(true);
	topLayout->addWidget(pbSciHyp);
	connect(pbSciHyp, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotHyptoggled(bool)));

	pbSciFE = new CalcButton("F-E", mScientificPage, "SciFE-Button", TQString("Fixed to Exponential"));
	pbSciFE->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbSciFE->setButtonType(CalcButton::TypeOther);
	pbSciFE->setToggleButton(true);
	topLayout->addWidget(pbSciFE);
	connect(pbSciFE, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotScientificFEtoggled(bool)));

	topLayout->addStretch(1);

	mScientificPageGrid = new TQWidget(parent);
	mScientificPageGrid->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	TQGridLayout *grid = new TQGridLayout(mScientificPageGrid, 7, 5, 0, 2);
	grid->setColStretch(0, 1);
	grid->setColStretch(1, 1);
	grid->setColStretch(2, 1);
	grid->setColStretch(3, 1);
	grid->setColStretch(4, 1);

	for(int i = 0; i < 7; ++i) grid->setRowStretch(i, 1);

	// Row 1: x^2, x^y, sin, cos, tan
	CalcButton *btnSciSq = new CalcButton(mScientificPageGrid, "SciSq-Button");
	btnSciSq->addMode(ModeNormal, "x<sup>2</sup>", TQString("Square"), true);
	btnSciSq->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciSq, 0, 0);
	connect(btnSciSq, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotSquareclicked(void)));

	CalcButton *btnSciPow = new CalcButton(mScientificPageGrid, "SciPow-Button");
	btnSciPow->addMode(ModeNormal, "x<sup>y</sup>", TQString("Power"), true);
	btnSciPow->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciPow, 0, 1);
	connect(btnSciPow, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPowerclicked(void)));

	pbSciSin = new CalcButton(mScientificPageGrid, "SciSin-Button");
	pbSciSin->addMode(ModeNormal, "sin", TQString("Sine"));
	pbSciSin->addMode(ModeInverse, "asin", TQString("Arc sine"));
	pbSciSin->addMode(ModeHyperbolic, "sinh", TQString("Hyperbolic sine"));
	pbSciSin->addMode(ButtonModeFlags(ModeInverse | ModeHyperbolic),
			"asinh", TQString("Inverse hyperbolic sine"));
	pbSciSin->setButtonType(CalcButton::TypeOperator);
	pbSciSin->setAccel(Key_S);
	grid->addWidget(pbSciSin, 0, 2);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciSin, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		pbSciSin, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(pbSciSin, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotSinclicked(void)));

	pbSciCos = new CalcButton(mScientificPageGrid, "SciCos-Button");
	pbSciCos->addMode(ModeNormal, "cos", TQString("Cosine"));
	pbSciCos->addMode(ModeInverse, "acos", TQString("Arc cosine"));
	pbSciCos->addMode(ModeHyperbolic, "cosh", TQString("Hyperbolic cosine"));
	pbSciCos->addMode(ButtonModeFlags(ModeInverse | ModeHyperbolic),
			"acosh", TQString("Inverse hyperbolic cosine"));
	pbSciCos->setButtonType(CalcButton::TypeOperator);
	pbSciCos->setAccel(Key_C);
	grid->addWidget(pbSciCos, 0, 3);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciCos, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		pbSciCos, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(pbSciCos, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotCosclicked(void)));

	pbSciTan = new CalcButton(mScientificPageGrid, "SciTan-Button");
	pbSciTan->addMode(ModeNormal, "tan", TQString("Tangent"));
	pbSciTan->addMode(ModeInverse, "atan", TQString("Arc tangent"));
	pbSciTan->addMode(ModeHyperbolic, "tanh", TQString("Hyperbolic tangent"));
	pbSciTan->addMode(ButtonModeFlags(ModeInverse | ModeHyperbolic),
			"atanh", TQString("Inverse hyperbolic tangent"));
	pbSciTan->setButtonType(CalcButton::TypeOperator);
	pbSciTan->setAccel(Key_T);
	grid->addWidget(pbSciTan, 0, 4);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciTan, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	connect(this, TQ_SIGNAL(switchMode(ButtonModeFlags,bool)),
		pbSciTan, TQ_SLOT(slotSetMode(ButtonModeFlags,bool)));
	connect(pbSciTan, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotTanclicked(void)));

	// Row 2: √, 10^x, log, Exp, Mod
	CalcButton *btnSciRoot = new CalcButton(mScientificPageGrid, "SciRoot-Button");
	btnSciRoot->addMode(ModeNormal, "\xe2\x88\x9a", TQString("Square Root"));
	btnSciRoot->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciRoot, 1, 0);
	connect(btnSciRoot, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotRootclicked(void)));

	CalcButton *btnSci10x = new CalcButton(mScientificPageGrid, "Sci10x-Button");
	btnSci10x->addMode(ModeNormal, "10<sup>x</sup>", TQString("10 to power of x"), true);
	btnSci10x->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSci10x, 1, 1);
	connect(btnSci10x, TQ_SIGNAL(clicked(void)), TQ_SLOT(slot10xclicked(void)));

	CalcButton *btnSciLog = new CalcButton(mScientificPageGrid, "SciLog-Button");
	btnSciLog->addMode(ModeNormal, "log", TQString("Logarithm base 10"));
	btnSciLog->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciLog, 1, 2);
	connect(btnSciLog, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotLogclicked(void)));

	CalcButton *btnSciExp = new CalcButton("Exp", mScientificPageGrid, "SciExp-Button", TQString("Exponent"));
	btnSciExp->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciExp, 1, 3);
	connect(btnSciExp, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotEEclicked(void)));

	CalcButton *btnSciMod = new CalcButton("Mod", mScientificPageGrid, "SciMod-Button", TQString("Modulo"));
	btnSciMod->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciMod, 1, 4);
	connect(btnSciMod, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotModclicked(void)));

	// Row 3: ↑, CE, C, Backspace, ÷
	pbSciInv = new CalcButton("\xe2\x86\x91", mScientificPageGrid, "SciInv-Button", TQString("Inverse"));
	pbSciInv->setToggleButton(true);
	pbSciInv->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(pbSciInv, 2, 0);
	connect(pbSciInv, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotInvtoggled(bool)));

	pbSciCE = new CalcButton("CE", mScientificPageGrid, "SciCE-Button", TQString("Clear Entry"));
	pbSciCE->setButtonType(CalcButton::TypeOperator);
	pbSciCE->setAccel(Key_Delete);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciCE, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciCE, 2, 1);
	connect(pbSciCE, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotClearclicked(void)));

	pbSciC = new CalcButton("C", mScientificPageGrid, "SciC-Button", TQString("Clear"));
	pbSciC->setButtonType(CalcButton::TypeOperator);
	pbSciC->setAccel(Key_Prior);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciC, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciC, 2, 2);
	connect(pbSciC, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotACclicked(void)));

	pbSciBS = new CalcButton("\xe2\x8c\xab", mScientificPageGrid, "SciBS-Button", TQString("Backspace"));
	pbSciBS->setButtonType(CalcButton::TypeOperator);
	pbSciBS->setAccel(Key_Backspace);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciBS, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciBS, 2, 3);
	connect(pbSciBS, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotBackSpaceclicked(void)));

	pbSciDiv = new CalcButton("\xc3\xb7", mScientificPageGrid, "SciDiv-Button", TQString("Division"));
	pbSciDiv->setButtonType(CalcButton::TypeOperator);
	pbSciDiv->setAccel(Key_Slash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciDiv, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciDiv, 2, 4);
	connect(pbSciDiv, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotDivisionclicked(void)));

	// Row 4: π, 7, 8, 9, ×
	CalcButton *btnSciPi = new CalcButton("\xcf\x80", mScientificPageGrid, "SciPi-Button", TQString("Pi"));
	btnSciPi->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciPi, 3, 0);
	connect(btnSciPi, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPiclicked(void)));

	CalcButton *btnSci7 = new CalcButton("7", mScientificPageGrid, "Sci7-Button", TQString("7"));
	btnSci7->setButtonType(CalcButton::TypeDigit);
	btnSci7->setAccel(Key_7);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci7, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci7, 3, 1);

	CalcButton *btnSci8 = new CalcButton("8", mScientificPageGrid, "Sci8-Button", TQString("8"));
	btnSci8->setButtonType(CalcButton::TypeDigit);
	btnSci8->setAccel(Key_8);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci8, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci8, 3, 2);

	CalcButton *btnSci9 = new CalcButton("9", mScientificPageGrid, "Sci9-Button", TQString("9"));
	btnSci9->setButtonType(CalcButton::TypeDigit);
	btnSci9->setAccel(Key_9);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci9, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci9, 3, 3);

	pbSciMul = new CalcButton("\xc3\x97", mScientificPageGrid, "SciMul-Button", TQString("Multiplication"));
	pbSciMul->setButtonType(CalcButton::TypeOperator);
	pbSciMul->setAccel(Key_multiply);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciMul, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciMul, 3, 4);
	connect(pbSciMul, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotXclicked(void)));

	// Row 5: n!, 4, 5, 6, -
	CalcButton *btnSciFact = new CalcButton("n!", mScientificPageGrid, "SciFact-Button", TQString("Factorial"));
	btnSciFact->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnSciFact, 4, 0);
	connect(btnSciFact, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotFactorialclicked(void)));
	
	CalcButton *btnSci4 = new CalcButton("4", mScientificPageGrid, "Sci4-Button", TQString("4"));
	btnSci4->setButtonType(CalcButton::TypeDigit);
	btnSci4->setAccel(Key_4);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci4, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci4, 4, 1);

	CalcButton *btnSci5 = new CalcButton("5", mScientificPageGrid, "Sci5-Button", TQString("5"));
	btnSci5->setButtonType(CalcButton::TypeDigit);
	btnSci5->setAccel(Key_5);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci5, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci5, 4, 2);

	CalcButton *btnSci6 = new CalcButton("6", mScientificPageGrid, "Sci6-Button", TQString("6"));
	btnSci6->setButtonType(CalcButton::TypeDigit);
	btnSci6->setAccel(Key_6);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci6, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci6, 4, 3);

	pbSciSub = new CalcButton("-", mScientificPageGrid, "SciSub-Button", TQString("Subtraction"));
	pbSciSub->setButtonType(CalcButton::TypeOperator);
	pbSciSub->setAccel(Key_Minus);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciSub, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciSub, 4, 4);
	connect(pbSciSub, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMinusclicked(void)));

	// Row 6: ±, 1, 2, 3, +
	pbSciPM = new CalcButton("\xc2\xb1", mScientificPageGrid, "SciPM-Button", TQString("Change Sign"));
	pbSciPM->setButtonType(CalcButton::TypeOperator);
	pbSciPM->setAccel(Key_Backslash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciPM, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciPM, 5, 0);
	connect(pbSciPM, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusMinusclicked(void)));

	CalcButton *btnSci1 = new CalcButton("1", mScientificPageGrid, "Sci1-Button", TQString("1"));
	btnSci1->setButtonType(CalcButton::TypeDigit);
	btnSci1->setAccel(Key_1);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci1, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci1, 5, 1);

	CalcButton *btnSci2 = new CalcButton("2", mScientificPageGrid, "Sci2-Button", TQString("2"));
	btnSci2->setButtonType(CalcButton::TypeDigit);
	btnSci2->setAccel(Key_2);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci2, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci2, 5, 2);

	CalcButton *btnSci3 = new CalcButton("3", mScientificPageGrid, "Sci3-Button", TQString("3"));
	btnSci3->setButtonType(CalcButton::TypeDigit);
	btnSci3->setAccel(Key_3);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci3, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci3, 5, 3);

	pbSciAdd = new CalcButton("+", mScientificPageGrid, "SciAdd-Button", TQString("Addition"));
	pbSciAdd->setButtonType(CalcButton::TypeOperator);
	pbSciAdd->setAccel(Key_Plus);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciAdd, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciAdd, 5, 4);
	connect(pbSciAdd, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusclicked(void)));

	// Row 7: (, ), 0, ., =
	pbSciParenL = new CalcButton("(", mScientificPageGrid, "SciParenL-Button", TQString("Left Parenthesis"));
	pbSciParenL->setButtonType(CalcButton::TypeOperator);
	pbSciParenL->setAccel(Key_ParenLeft);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciParenL, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciParenL, 6, 0);
	connect(pbSciParenL, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotParenOpenclicked(void)));

	pbSciParenR = new CalcButton(")", mScientificPageGrid, "SciParenR-Button", TQString("Right Parenthesis"));
	pbSciParenR->setButtonType(CalcButton::TypeOperator);
	pbSciParenR->setAccel(Key_ParenRight);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciParenR, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciParenR, 6, 1);
	connect(pbSciParenR, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotParenCloseclicked(void)));

	CalcButton *btnSci0 = new CalcButton("0", mScientificPageGrid, "Sci0-Button", TQString("0"));
	btnSci0->setButtonType(CalcButton::TypeDigit);
	btnSci0->setAccel(Key_0);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnSci0, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnSci0, 6, 2);

	pbSciDot = new CalcButton(".", mScientificPageGrid, "SciDot-Button", TQString("Decimal Point"));
	pbSciDot->setButtonType(CalcButton::TypeOperator);
	pbSciDot->setAccel(Key_Period);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciDot, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciDot, 6, 3);
	connect(pbSciDot, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPeriodclicked(void)));

	pbSciEq = new CalcButton("=", mScientificPageGrid, "SciEq-Button", TQString("Equals"));
	pbSciEq->setButtonType(CalcButton::TypeEqual);
	pbSciEq->setAccel(Key_Equal);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbSciEq, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbSciEq, 6, 4);
	connect(pbSciEq, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotEqualclicked(void)));

	SciNumButtonGroup->insert(btnSci0, 0);
	SciNumButtonGroup->insert(btnSci1, 1);
	SciNumButtonGroup->insert(btnSci2, 2);
	SciNumButtonGroup->insert(btnSci3, 3);
	SciNumButtonGroup->insert(btnSci4, 4);
	SciNumButtonGroup->insert(btnSci5, 5);
	SciNumButtonGroup->insert(btnSci6, 6);
	SciNumButtonGroup->insert(btnSci7, 7);
	SciNumButtonGroup->insert(btnSci8, 8);
	SciNumButtonGroup->insert(btnSci9, 9);

	if (const TQObjectList *l = mScientificPageGrid->children()) {
		TQObjectListIt it(*l);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			if (obj->inherits("CalcButton")) {
				((CalcButton*)obj)->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
			}
		}
	}

	applyIconToButton(pbSciAdd, plus_png, plus_png_len, TQString("Addition"));
	applyIconToButton(pbSciSub, minus_png, minus_png_len, TQString("Subtraction"));
	applyIconToButton(pbSciMul, multiply_png, multiply_png_len, TQString("Multiplication"));
	applyIconToButton(pbSciDiv, divide_png, divide_png_len, TQString("Division"));
	applyIconToButton(pbSciEq, equal_png, equal_png_len, TQString("Equals"));
	applyIconToButton(pbSciPM, plusminus_png, plusminus_png_len, TQString("Change Sign"));
	applyIconToButton(pbSciBS, backspace_png, backspace_png_len, TQString("Backspace"));
	applyIconToButton(btnSciRoot, squareroot_png, squareroot_png_len, TQString("Square Root"));
	applyIconToButton(btnSciSq, square_png, square_png_len, TQString("Square"));
	applyIconToButton(btnSciFact, nfact_png, nfact_png_len, TQString("Factorial"));
	applyIconToButton(btnSciPi, pi_png, pi_png_len, TQString("Pi"));
	applyIconToButton(pbSciInv, uparrow_png, uparrow_png_len, TQString("Inverse"));
	applyIconToButton(btnSciPow, xexpy_png, xexpy_png_len, TQString("Power"));
	applyIconToButton(btnSci10x, tenexpx_png, tenexpx_png_len, TQString("10 to power of x"));

	mScientificPage->hide();
	mScientificPageGrid->hide();
	return mScientificPageGrid;
}

void Calculator::slotSciAngleClicked(void)
{
	// Cycle through DEG -> RAD -> GRAD -> DEG
	int current = _angle_mode;
	current = (current + 1) % 3;
	slotAngleSelected(current);
}

void Calculator::slotScientificFEtoggled(bool toggled)
{
	calc_display->setScientificFormat(toggled);
}

void Calculator::slotWordSizeClicked(void)
{
	_word_size = (_word_size + 1) % 4;
	if (_word_size == 0) pbProgWordSize->setText(TQString("QWORD"));
	else if (_word_size == 1) pbProgWordSize->setText(TQString("DWORD"));
	else if (_word_size == 2) pbProgWordSize->setText(TQString("WORD"));
	else if (_word_size == 3) pbProgWordSize->setText(TQString("BYTE"));

	calc_display->setWordSize(_word_size);
	updateBitboard();
}

void Calculator::slotKeypadViewClicked(void)
{
	m_programmer_bitboard_view = false;
	if (m_programmerMemoryFrameVisible) return;
	if (mBitboardPage) mBitboardPage->hide();
	TQPtrListIterator<TQWidget> it(mProgKeypadWidgets);
	TQWidget *w;
	while ((w = it.current()) != 0) {
		++it;
		w->show();
	}
}

void Calculator::slotBitboardViewClicked(void)
{
	m_programmer_bitboard_view = true;
	if (m_programmerMemoryFrameVisible) return;
	TQPtrListIterator<TQWidget> it(mProgKeypadWidgets);
	TQWidget *w;
	while ((w = it.current()) != 0) {
		++it;
		w->hide();
	}
	if (mBitboardPage) mBitboardPage->show();
	updateBitboard();
	updateBitboardLabelSizes();
}

void Calculator::updateBitboard(void)
{
	if (!mBitboardPage || !mBitboardPage->isVisible()) return;

	unsigned long long val = static_cast<unsigned long long>(calc_display->getAmount());
	for (int i = 0; i < 64; ++i) {
		CalcButton *btn = (CalcButton*)BitboardButtonGroup->find(i);
		if (btn) {
			if ((val >> i) & 1) {
				btn->setText("1");
			} else {
				btn->setText("0");
			}
			
			if (_word_size == 1 && i >= 32) btn->setEnabled(false);
			else if (_word_size == 2 && i >= 16) btn->setEnabled(false);
			else if (_word_size == 3 && i >= 8) btn->setEnabled(false);
			else btn->setEnabled(true);
		}
	}

	if (mBitboardLabels[0]) {
		for (int r = 0; r < 4; ++r) {
			bool enabled = true;
			if (_word_size == 1 && r < 2) enabled = false;
			else if (_word_size == 2 && r < 3) enabled = false;
			else if (_word_size == 3 && r < 3) enabled = false;

			for (int g = 0; g < 4; ++g) {
				mBitboardLabels[r * 4 + g]->setEnabled(enabled);
			}
		}
	}
	updateBitboardLabelSizes();
}

void Calculator::updateBitboardLabelSizes(void)
{
	if (!mBitboardPage || !mBitboardLabels[0] || !mBitboardPage->isVisible()) return;

	int h = height();
	if (h < 50) return;

	// Scale font size linearly based on window height (independent of child widget sizeHints)
	int font_size = h * 0.020;
	if (font_size < 8) font_size = 8;
	if (font_size > 14) font_size = 14;

	TQFont f = mBitboardPage->font();
	f.setPixelSize(font_size);
	f.setBold(false);

	for (int i = 0; i < 16; ++i) {
		if (mBitboardLabels[i]) {
			mBitboardLabels[i]->setFont(f);
		}
	}
}

void Calculator::slotBitboardClicked(int id)
{
	unsigned long long val = static_cast<unsigned long long>(calc_display->getAmount());
	val ^= (1ULL << id);
	if (_word_size == 1) val = static_cast<uint32_t>(val);
	else if (_word_size == 2) val = static_cast<uint16_t>(val);
	else if (_word_size == 3) val = static_cast<uint8_t>(val);
	
	calc_display->setAmount(KNumber(static_cast<unsigned long long>(val)));
	updateBitboard();
}

TQWidget* Calculator::setupProgrammerKeys_win10(TQWidget *parent)
{
	mProgrammerPage = new TQWidget(parent);
	ProgNumButtonGroup = new TQButtonGroup(this, "Prog-Num-Button-Group");
	ProgNumButtonGroup->hide();
	connect(ProgNumButtonGroup, TQ_SIGNAL(clicked(int)), TQ_SLOT(slotNumberclicked(int)));
	TQVBoxLayout *pageLayout = new TQVBoxLayout(mProgrammerPage, 0, mInternalSpacing);
	TQHBoxLayout *topLayout = new TQHBoxLayout();
	pageLayout->addLayout(topLayout, 1);

	// Move the existing BaseChooseGroup into the programmer page
	BaseChooseGroup->reparent(mProgrammerPage, TQPoint(0,0));
	topLayout->addWidget(BaseChooseGroup);
	topLayout->addStretch(1);

	mProgrammerPageGrid = new TQWidget(parent);
	mProgrammerPageGrid->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	TQGridLayout *grid = new TQGridLayout(mProgrammerPageGrid, 7, 6, 0, 2);
	for(int i = 0; i < 6; ++i) grid->setColStretch(i, 1);
	for(int i = 0; i < 7; ++i) grid->setRowStretch(i, 1);

	// Row 0: Toolbar items (Keypad, Bitboard, WordSize, MS)
	CalcButton *btnProgKeypad = new CalcButton("", mProgrammerPageGrid, "ProgKeypad-Button", TQString("Keypad View"));
	btnProgKeypad->setButtonType(CalcButton::TypeOperator);
	btnProgKeypad->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	btnProgKeypad->setFlat(true);
	grid->addWidget(btnProgKeypad, 0, 0);
	connect(btnProgKeypad, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotKeypadViewClicked(void)));

	CalcButton *btnProgBitboard = new CalcButton("", mProgrammerPageGrid, "ProgBitboard-Button", TQString("Bitboard View"));
	btnProgBitboard->setButtonType(CalcButton::TypeOperator);
	btnProgBitboard->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	btnProgBitboard->setFlat(true);
	grid->addWidget(btnProgBitboard, 0, 1);
	connect(btnProgBitboard, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotBitboardViewClicked(void)));

	pbProgWordSize = new CalcButton("QWORD", mProgrammerPageGrid, "ProgWordSize-Button", TQString("Word Size"));
	pbProgWordSize->setButtonType(CalcButton::TypeOperator);
	pbProgWordSize->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	pbProgWordSize->setFlat(true);
	grid->addWidget(pbProgWordSize, 0, 2);
	connect(pbProgWordSize, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotWordSizeClicked(void)));

	CalcButton *btnProgMS = new CalcButton("MS", mProgrammerPageGrid, "ProgMS-Button", TQString("Memory Store"));
	btnProgMS->setButtonType(CalcButton::TypeOperator);
	btnProgMS->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	btnProgMS->setFlat(true);
	grid->addWidget(btnProgMS, 0, 4);
	connect(btnProgMS, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotProgMemStoreclicked(void)));

	btnProgM = new CalcButton(TQString::fromUtf8("M\xe2\x96\xbc"), mProgrammerPageGrid, "ProgM-Button", TQString("Memory List"));
	btnProgM->setButtonType(CalcButton::TypeOperator);
	btnProgM->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	btnProgM->setFlat(true);
	btnProgM->setEnabled(false);
	grid->addWidget(btnProgM, 0, 5);
	connect(btnProgM, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotToggleProgMemoryFrame(void)));

	mProgrammerMemoryFrame = createMemoryFrame(mProgrammerPageGrid, mProgMemoryFrameScroll);
	grid->addMultiCellWidget(mProgrammerMemoryFrame, 1, 6, 0, 5);
	mProgrammerMemoryFrame->hide();

	mBitboardPage = new TQWidget(mProgrammerPageGrid);
	mBitboardPage->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	grid->addMultiCellWidget(mBitboardPage, 1, 6, 0, 5);

	TQGridLayout *bitGrid = new TQGridLayout(mBitboardPage, 8, 22, 0, 2);
	BitboardButtonGroup = new TQButtonGroup(this, "Bitboard-Button-Group");
	BitboardButtonGroup->hide();
	connect(BitboardButtonGroup, TQ_SIGNAL(clicked(int)), TQ_SLOT(slotBitboardClicked(int)));
	for (int i = 0; i < 64; ++i) {
		int row = 3 - (i / 16);
		int col = 18 - (i % 16);
		if (col < 4) col += 0;
		else if (col < 8) col += 1;
		else if (col < 12) col += 2;
		else col += 3;
		CalcButton *btnBit = new CalcButton("0", mBitboardPage, TQString("Bit-%1").arg(i).latin1());
		btnBit->setButtonType(CalcButton::TypeDigit);
		btnBit->setFlat(true);
		btnBit->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
		BitboardButtonGroup->insert(btnBit, i);
		bitGrid->addWidget(btnBit, row * 2, col);
	}

	TQFont labelFont = mBitboardPage->font();
	labelFont.setPointSize(8);
	labelFont.setBold(false);

	for (int r = 0; r < 4; ++r) {
		// Group 3 rightmost bit (bit 60 - r*16)
		int val_g3 = 60 - r * 16;
		TQLabel *lbl_g3 = new TQLabel(TQString::number(val_g3), mBitboardPage);
		lbl_g3->setFont(labelFont);
		lbl_g3->setAlignment(AlignCenter);
		bitGrid->addWidget(lbl_g3, r * 2 + 1, 7);
		mBitboardLabels[r * 4] = lbl_g3;

		// Group 2 rightmost bit (bit 56 - r*16)
		int val_g2 = 56 - r * 16;
		TQLabel *lbl_g2 = new TQLabel(TQString::number(val_g2), mBitboardPage);
		lbl_g2->setFont(labelFont);
		lbl_g2->setAlignment(AlignCenter);
		bitGrid->addWidget(lbl_g2, r * 2 + 1, 12);
		mBitboardLabels[r * 4 + 1] = lbl_g2;

		// Group 1 rightmost bit (bit 52 - r*16)
		int val_g1 = 52 - r * 16;
		TQLabel *lbl_g1 = new TQLabel(TQString::number(val_g1), mBitboardPage);
		lbl_g1->setFont(labelFont);
		lbl_g1->setAlignment(AlignCenter);
		bitGrid->addWidget(lbl_g1, r * 2 + 1, 17);
		mBitboardLabels[r * 4 + 2] = lbl_g1;

		// Group 0 rightmost bit (bit 48 - r*16)
		int val_g0 = 48 - r * 16;
		TQLabel *lbl_g0 = new TQLabel(TQString::number(val_g0), mBitboardPage);
		lbl_g0->setFont(labelFont);
		lbl_g0->setAlignment(AlignCenter);
		bitGrid->addWidget(lbl_g0, r * 2 + 1, 21);
		mBitboardLabels[r * 4 + 3] = lbl_g0;
	}

	for (int col : {4, 9, 14}) bitGrid->setColStretch(col, 1);
	for (int r = 0; r < 4; ++r) {
		bitGrid->setRowStretch(r * 2, 1);
		bitGrid->setRowStretch(r * 2 + 1, 0);
	}
	mBitboardPage->hide();

	// Row 1: Lsh, Rsh, Or, Xor, Not, And
	CalcButton *btnProgLsh = new CalcButton("Lsh", mProgrammerPageGrid, "ProgLsh-Button", TQString("Left Shift"));
	btnProgLsh->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgLsh, 1, 0);
	connect(btnProgLsh, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotLeftShiftclicked(void)));

	CalcButton *btnProgRsh = new CalcButton("Rsh", mProgrammerPageGrid, "ProgRsh-Button", TQString("Right Shift"));
	btnProgRsh->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgRsh, 1, 1);
	connect(btnProgRsh, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotRightShiftclicked(void)));

	CalcButton *btnProgOr = new CalcButton("Or", mProgrammerPageGrid, "ProgOr-Button", TQString("Logical OR"));
	btnProgOr->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgOr, 1, 2);
	connect(btnProgOr, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotORclicked(void)));

	CalcButton *btnProgXor = new CalcButton("Xor", mProgrammerPageGrid, "ProgXor-Button", TQString("Logical XOR"));
	btnProgXor->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgXor, 1, 3);
	connect(btnProgXor, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotXORclicked(void)));

	CalcButton *btnProgNot = new CalcButton("Not", mProgrammerPageGrid, "ProgNot-Button", TQString("Logical NOT"));
	btnProgNot->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgNot, 1, 4);
	connect(btnProgNot, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotNegateclicked(void)));

	CalcButton *btnProgAnd = new CalcButton("And", mProgrammerPageGrid, "ProgAnd-Button", TQString("Logical AND"));
	btnProgAnd->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgAnd, 1, 5);
	connect(btnProgAnd, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotANDclicked(void)));

	// Row 1: ↑ (Inv), Mod, CE, C, Backspace, ÷
	CalcButton *btnProgInv = new CalcButton("\xe2\x86\x91", mProgrammerPageGrid, "ProgInv-Button", TQString("Inverse"));
	btnProgInv->setToggleButton(true);
	btnProgInv->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgInv, 2, 0);
	connect(btnProgInv, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotInvtoggled(bool)));

	CalcButton *btnProgMod = new CalcButton("Mod", mProgrammerPageGrid, "ProgMod-Button", TQString("Modulo"));
	btnProgMod->setButtonType(CalcButton::TypeOperator);
	grid->addWidget(btnProgMod, 2, 1);
	connect(btnProgMod, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotModclicked(void)));

	pbProgCE = new CalcButton("CE", mProgrammerPageGrid, "ProgCE-Button", TQString("Clear Entry"));
	pbProgCE->setButtonType(CalcButton::TypeOperator);
	pbProgCE->setAccel(Key_Delete);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgCE, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgCE, 2, 2);
	connect(pbProgCE, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotClearclicked(void)));

	pbProgC = new CalcButton("C", mProgrammerPageGrid, "ProgC-Button", TQString("Clear"));
	pbProgC->setButtonType(CalcButton::TypeOperator);
	pbProgC->setAccel(Key_Prior);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgC, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgC, 2, 3);
	connect(pbProgC, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotACclicked(void)));

	pbProgBS = new CalcButton("\xe2\x8c\xab", mProgrammerPageGrid, "ProgBS-Button", TQString("Backspace"));
	pbProgBS->setButtonType(CalcButton::TypeOperator);
	pbProgBS->setAccel(Key_Backspace);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgBS, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgBS, 2, 4);
	connect(pbProgBS, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotBackSpaceclicked(void)));

	pbProgDiv = new CalcButton("\xc3\xb7", mProgrammerPageGrid, "ProgDiv-Button", TQString("Division"));
	pbProgDiv->setButtonType(CalcButton::TypeOperator);
	pbProgDiv->setAccel(Key_Slash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgDiv, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgDiv, 2, 5);
	connect(pbProgDiv, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotDivisionclicked(void)));

	// Row 2: A, B, 7, 8, 9, ×
	CalcButton *btnProgA = new CalcButton("A", mProgrammerPageGrid, "ProgA-Button", TQString("A"));
	btnProgA->setButtonType(CalcButton::TypeDigit);
	btnProgA->setAccel(Key_A);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProgA, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProgA, 3, 0);

	CalcButton *btnProgB = new CalcButton("B", mProgrammerPageGrid, "ProgB-Button", TQString("B"));
	btnProgB->setButtonType(CalcButton::TypeDigit);
	btnProgB->setAccel(Key_B);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProgB, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProgB, 3, 1);

	CalcButton *btnProg7 = new CalcButton("7", mProgrammerPageGrid, "Prog7-Button", TQString("7"));
	btnProg7->setButtonType(CalcButton::TypeDigit);
	btnProg7->setAccel(Key_7);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg7, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg7, 3, 2);

	CalcButton *btnProg8 = new CalcButton("8", mProgrammerPageGrid, "Prog8-Button", TQString("8"));
	btnProg8->setButtonType(CalcButton::TypeDigit);
	btnProg8->setAccel(Key_8);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg8, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg8, 3, 3);

	CalcButton *btnProg9 = new CalcButton("9", mProgrammerPageGrid, "Prog9-Button", TQString("9"));
	btnProg9->setButtonType(CalcButton::TypeDigit);
	btnProg9->setAccel(Key_9);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg9, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg9, 3, 4);

	pbProgMul = new CalcButton("\xc3\x97", mProgrammerPageGrid, "ProgMul-Button", TQString("Multiplication"));
	pbProgMul->setButtonType(CalcButton::TypeOperator);
	pbProgMul->setAccel(Key_multiply);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgMul, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgMul, 3, 5);
	connect(pbProgMul, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotXclicked(void)));

	// Row 3: C, D, 4, 5, 6, -
	CalcButton *btnProgDigitC = new CalcButton("C", mProgrammerPageGrid, "ProgDigitC-Button", TQString("C"));
	btnProgDigitC->setButtonType(CalcButton::TypeDigit);
	btnProgDigitC->setAccel(Key_C);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProgDigitC, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProgDigitC, 4, 0);

	CalcButton *btnProgD = new CalcButton("D", mProgrammerPageGrid, "ProgD-Button", TQString("D"));
	btnProgD->setButtonType(CalcButton::TypeDigit);
	btnProgD->setAccel(Key_D);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProgD, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProgD, 4, 1);

	CalcButton *btnProg4 = new CalcButton("4", mProgrammerPageGrid, "Prog4-Button", TQString("4"));
	btnProg4->setButtonType(CalcButton::TypeDigit);
	btnProg4->setAccel(Key_4);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg4, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg4, 4, 2);

	CalcButton *btnProg5 = new CalcButton("5", mProgrammerPageGrid, "Prog5-Button", TQString("5"));
	btnProg5->setButtonType(CalcButton::TypeDigit);
	btnProg5->setAccel(Key_5);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg5, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg5, 4, 3);

	CalcButton *btnProg6 = new CalcButton("6", mProgrammerPageGrid, "Prog6-Button", TQString("6"));
	btnProg6->setButtonType(CalcButton::TypeDigit);
	btnProg6->setAccel(Key_6);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg6, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg6, 4, 4);

	pbProgSub = new CalcButton("-", mProgrammerPageGrid, "ProgSub-Button", TQString("Subtraction"));
	pbProgSub->setButtonType(CalcButton::TypeOperator);
	pbProgSub->setAccel(Key_Minus);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgSub, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgSub, 4, 5);
	connect(pbProgSub, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotMinusclicked(void)));

	// Row 4: E, F, 1, 2, 3, +
	CalcButton *btnProgE = new CalcButton("E", mProgrammerPageGrid, "ProgE-Button", TQString("E"));
	btnProgE->setButtonType(CalcButton::TypeDigit);
	btnProgE->setAccel(Key_E);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProgE, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProgE, 5, 0);

	CalcButton *btnProgF = new CalcButton("F", mProgrammerPageGrid, "ProgF-Button", TQString("F"));
	btnProgF->setButtonType(CalcButton::TypeDigit);
	btnProgF->setAccel(Key_F);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProgF, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProgF, 5, 1);

	CalcButton *btnProg1 = new CalcButton("1", mProgrammerPageGrid, "Prog1-Button", TQString("1"));
	btnProg1->setButtonType(CalcButton::TypeDigit);
	btnProg1->setAccel(Key_1);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg1, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg1, 5, 2);

	CalcButton *btnProg2 = new CalcButton("2", mProgrammerPageGrid, "Prog2-Button", TQString("2"));
	btnProg2->setButtonType(CalcButton::TypeDigit);
	btnProg2->setAccel(Key_2);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg2, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg2, 5, 3);

	CalcButton *btnProg3 = new CalcButton("3", mProgrammerPageGrid, "Prog3-Button", TQString("3"));
	btnProg3->setButtonType(CalcButton::TypeDigit);
	btnProg3->setAccel(Key_3);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg3, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg3, 5, 4);

	pbProgAdd = new CalcButton("+", mProgrammerPageGrid, "ProgAdd-Button", TQString("Addition"));
	pbProgAdd->setButtonType(CalcButton::TypeOperator);
	pbProgAdd->setAccel(Key_Plus);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgAdd, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgAdd, 5, 5);
	connect(pbProgAdd, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusclicked(void)));

	// Row 5: (, ), ±, 0, ., =
	pbProgParenL = new CalcButton("(", mProgrammerPageGrid, "ProgParenL-Button", TQString("Left Parenthesis"));
	pbProgParenL->setButtonType(CalcButton::TypeOperator);
	pbProgParenL->setAccel(Key_ParenLeft);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgParenL, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgParenL, 6, 0);
	connect(pbProgParenL, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotParenOpenclicked(void)));

	pbProgParenR = new CalcButton(")", mProgrammerPageGrid, "ProgParenR-Button", TQString("Right Parenthesis"));
	pbProgParenR->setButtonType(CalcButton::TypeOperator);
	pbProgParenR->setAccel(Key_ParenRight);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgParenR, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgParenR, 6, 1);
	connect(pbProgParenR, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotParenCloseclicked(void)));

	pbProgPM = new CalcButton("\xc2\xb1", mProgrammerPageGrid, "ProgPM-Button", TQString("Change Sign"));
	pbProgPM->setButtonType(CalcButton::TypeOperator);
	pbProgPM->setAccel(Key_Backslash);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgPM, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgPM, 6, 2);
	connect(pbProgPM, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotPlusMinusclicked(void)));

	CalcButton *btnProg0 = new CalcButton("0", mProgrammerPageGrid, "Prog0-Button", TQString("0"));
	btnProg0->setButtonType(CalcButton::TypeDigit);
	btnProg0->setAccel(Key_0);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		btnProg0, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(btnProg0, 6, 3);

	CalcButton *btnProgDot = new CalcButton(".", mProgrammerPageGrid, "ProgDot-Button", TQString("Decimal Point"));
	btnProgDot->setButtonType(CalcButton::TypeOperator);
	btnProgDot->setEnabled(false); // In Programmer mode, no decimal
	grid->addWidget(btnProgDot, 6, 4);

	pbProgEq = new CalcButton("=", mProgrammerPageGrid, "ProgEq-Button", TQString("Equals"));
	pbProgEq->setButtonType(CalcButton::TypeEqual);
	pbProgEq->setAccel(Key_Equal);
	connect(this, TQ_SIGNAL(switchShowAccels(bool)),
		pbProgEq, TQ_SLOT(slotSetAccelDisplayMode(bool)));
	grid->addWidget(pbProgEq, 6, 5);
	connect(pbProgEq, TQ_SIGNAL(clicked(void)), TQ_SLOT(slotEqualclicked(void)));

	ProgNumButtonGroup->insert(btnProg0, 0);
	ProgNumButtonGroup->insert(btnProg1, 1);
	ProgNumButtonGroup->insert(btnProg2, 2);
	ProgNumButtonGroup->insert(btnProg3, 3);
	ProgNumButtonGroup->insert(btnProg4, 4);
	ProgNumButtonGroup->insert(btnProg5, 5);
	ProgNumButtonGroup->insert(btnProg6, 6);
	ProgNumButtonGroup->insert(btnProg7, 7);
	ProgNumButtonGroup->insert(btnProg8, 8);
	ProgNumButtonGroup->insert(btnProg9, 9);
	ProgNumButtonGroup->insert(btnProgA, 10);
	ProgNumButtonGroup->insert(btnProgB, 11);
	ProgNumButtonGroup->insert(btnProgDigitC, 12);
	ProgNumButtonGroup->insert(btnProgD, 13);
	ProgNumButtonGroup->insert(btnProgE, 14);
	ProgNumButtonGroup->insert(btnProgF, 15);

	if (const TQObjectList *l = mProgrammerPageGrid->children()) {
		TQObjectListIt it(*l);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			if (obj->inherits("CalcButton")) {
				CalcButton *cb = static_cast<CalcButton*>(obj);
				cb->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
				// Populate the keypad widget list (all non-toolbar buttons)
				if (cb != btnProgKeypad && cb != btnProgBitboard &&
				    cb != pbProgWordSize && cb != btnProgMS && cb != btnProgM) {
					mProgKeypadWidgets.append(cb);
				}
			}
		}
	}

	applyIconToButton(pbProgAdd, plus_png, plus_png_len, TQString("Addition"));
	applyIconToButton(pbProgSub, minus_png, minus_png_len, TQString("Subtraction"));
	applyIconToButton(pbProgMul, multiply_png, multiply_png_len, TQString("Multiplication"));
	applyIconToButton(pbProgDiv, divide_png, divide_png_len, TQString("Division"));
	applyIconToButton(pbProgEq, equal_png, equal_png_len, TQString("Equals"));
	applyIconToButton(pbProgPM, plusminus_png, plusminus_png_len, TQString("Change Sign"));
	applyIconToButton(pbProgBS, backspace_png, backspace_png_len, TQString("Backspace"));
	applyIconToButton(btnProgInv, uparrow_png, uparrow_png_len, TQString("Inverse"));
	applyIconToButton(btnProgKeypad, normal_keys_png, normal_keys_png_len, TQString("Keypad View"));
	applyIconToButton(btnProgBitboard, bitboard_keys_png, bitboard_keys_png_len, TQString("Bitboard View"));

	mProgrammerPage->hide();
	mProgrammerPageGrid->hide();
	return mProgrammerPageGrid;
}

/* ============================================================
 *  Date Calculation Page
 * ============================================================ */

void Calculator::setupDateCalcPage(TQWidget *parent)
{
	mDateCalcPage = new TQWidget(parent);
	TQVBoxLayout *pageLayout = new TQVBoxLayout(mDateCalcPage, 12, 8);

	// --- Mode combo: "Difference between dates" / "Add or subtract days" ---
	mDateModeCombo = new TQComboBox(false, mDateCalcPage, "DateMode-Combo");
	mDateModeCombo->insertItem(tr_str("Difference between dates"));
	mDateModeCombo->insertItem(tr_str("Add or subtract days"));
	mDateModeCombo->setCurrentItem(0);

	TQFont comboFont = mDateModeCombo->font();
	comboFont.setPointSize(comboFont.pointSize() + 1);
	mDateModeCombo->setFont(comboFont);
	mDateModeCombo->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Fixed);
	mDateModeCombo->setMinimumSize(1, 1);
	connect(mDateModeCombo, TQ_SIGNAL(activated(int)), TQ_SLOT(slotDateModeChanged(int)));
	pageLayout->addWidget(mDateModeCombo);

	// --- "From" section ---
	lblFrom = new TQLabel(tr_str("From"), mDateCalcPage);
	TQFont sectionFont = lblFrom->font();
	sectionFont.setPointSize(sectionFont.pointSize());
	lblFrom->setFont(sectionFont);
	lblFrom->setMinimumSize(1, 1);
	pageLayout->addWidget(lblFrom);

	mDateFromPicker = new TQtDatePeriodPicker(mDateCalcPage);
	mDateFromPicker->setDatePickerType(TQtDPPDayType);
	mDateFromPicker->setAllowedPickerTypes(TQtDPPDayType);
	mDateFromPicker->setDate(TQDate::currentDate());
	mDateFromPicker->setMinimumSize(1, 1);
	connect(mDateFromPicker, TQ_SIGNAL(dateChanged(const TQDate&)),
	        TQ_SLOT(slotDateFromChanged(const TQDate&)));
	pageLayout->addWidget(mDateFromPicker);

	// --- Mode 1: Radio buttons for Add / Subtract ---
	mRadioLayout = new TQHBoxLayout();
	mRadioLayout->setSpacing(8);
	mAddRadio = new TQRadioButton(tr_str("Add"), mDateCalcPage);
	mAddRadio->setChecked(true);
	mSubRadio = new TQRadioButton(tr_str("Subtract"), mDateCalcPage);
	mRadioLayout->addWidget(mAddRadio);
	mRadioLayout->addWidget(mSubRadio);
	pageLayout->addLayout(mRadioLayout);

	TQButtonGroup *dateCalcRadioGroup = new TQButtonGroup(mDateCalcPage);
	dateCalcRadioGroup->hide();
	dateCalcRadioGroup->insert(mAddRadio);
	dateCalcRadioGroup->insert(mSubRadio);

	connect(mAddRadio, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotDateInputChanged()));
	connect(mSubRadio, TQ_SIGNAL(toggled(bool)), TQ_SLOT(slotDateInputChanged()));

	// --- Mode 1: Offsets (Years, Months, Days) ---
	mOffsetLayout = new TQHBoxLayout();
	mOffsetLayout->setSpacing(8);

	colYears = new TQVBoxLayout();
	lblYears = new TQLabel(tr_str("years"), mDateCalcPage);
	lblYears->setFont(sectionFont);
	mYearsCombo = new TQSpinBox(0, 999, 1, mDateCalcPage);
	mYearsCombo->setValue(0);
	colYears->addWidget(lblYears);
	colYears->addWidget(mYearsCombo);

	colMonths = new TQVBoxLayout();
	lblMonths = new TQLabel(tr_str("months"), mDateCalcPage);
	lblMonths->setFont(sectionFont);
	mMonthsCombo = new TQSpinBox(0, 999, 1, mDateCalcPage);
	mMonthsCombo->setValue(0);
	colMonths->addWidget(lblMonths);
	colMonths->addWidget(mMonthsCombo);

	colDays = new TQVBoxLayout();
	lblDays = new TQLabel(tr_str("days"), mDateCalcPage);
	lblDays->setFont(sectionFont);
	mDaysCombo = new TQSpinBox(0, 999, 1, mDateCalcPage);
	mDaysCombo->setValue(0);
	colDays->addWidget(lblDays);
	colDays->addWidget(mDaysCombo);

	mOffsetLayout->addLayout(colYears);
	mOffsetLayout->addLayout(colMonths);
	mOffsetLayout->addLayout(colDays);
	pageLayout->addLayout(mOffsetLayout);

	connect(mYearsCombo, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotDateInputChanged()));
	connect(mMonthsCombo, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotDateInputChanged()));
	connect(mDaysCombo, TQ_SIGNAL(valueChanged(int)), TQ_SLOT(slotDateInputChanged()));

	// Hide Mode 1 widgets by default
	mAddRadio->hide();
	mSubRadio->hide();
	lblYears->hide();
	mYearsCombo->hide();
	lblMonths->hide();
	mMonthsCombo->hide();
	lblDays->hide();
	mDaysCombo->hide();

	// --- "To" section ---
	lblTo = new TQLabel(tr_str("To"), mDateCalcPage);
	lblTo->setFont(sectionFont);
	lblTo->setMinimumSize(1, 1);
	pageLayout->addWidget(lblTo);

	mDateToPicker = new TQtDatePeriodPicker(mDateCalcPage);
	mDateToPicker->setDatePickerType(TQtDPPDayType);
	mDateToPicker->setAllowedPickerTypes(TQtDPPDayType);
	mDateToPicker->setDate(TQDate::currentDate());
	mDateToPicker->setMinimumSize(1, 1);
	connect(mDateToPicker, TQ_SIGNAL(dateChanged(const TQDate&)),
	        TQ_SLOT(slotDateToChanged(const TQDate&)));
	pageLayout->addWidget(mDateToPicker);

	// --- Separator ---
	TQFrame *separator = new TQFrame(mDateCalcPage);
	separator->setFrameShape(TQFrame::HLine);
	separator->setFrameShadow(TQFrame::Sunken);
	pageLayout->addWidget(separator);

	// --- "Difference" section ---
	mDateDiffLabel = new TQLabel(tr_str("Difference"), mDateCalcPage);
	mDateDiffLabel->setFont(sectionFont);
	mDateDiffLabel->setMinimumSize(1, 1);
	pageLayout->addWidget(mDateDiffLabel);

	mDateDiffResult = new TQLabel(tr_str("Same dates"), mDateCalcPage);
	TQFont resultFont = mDateDiffResult->font();
	resultFont.setBold(true);
	resultFont.setPointSize(resultFont.pointSize() + 4);
	mDateDiffResult->setFont(resultFont);
	mDateDiffResult->setMinimumSize(1, 1);
	pageLayout->addWidget(mDateDiffResult);

	mDateDiffResultDays = new TQLabel("", mDateCalcPage);
	TQFont daysFont = mDateDiffResultDays->font();
	daysFont.setItalic(true);
	mDateDiffResultDays->setFont(daysFont);
	mDateDiffResultDays->setMinimumSize(1, 1);
	pageLayout->addWidget(mDateDiffResultDays);

	mDateDiffResult->installEventFilter(this);
	mDateDiffResultDays->installEventFilter(this);

	pageLayout->addStretch(1);

	// Apply background palette to match other modes
	TQColor bgColor(230, 230, 230);
	TQPalette pal = mDateCalcPage->palette();
	pal.setColor(TQPalette::Active, TQColorGroup::Background, bgColor);
	pal.setColor(TQPalette::Inactive, TQColorGroup::Background, bgColor);
	mDateCalcPage->setPalette(pal);
	mDateCalcPage->setPaletteBackgroundColor(bgColor);

	mDateCalcPage->hide();
}

void Calculator::slotDateModeChanged(int index)
{
	bool is_addsub = (index == 1);

	// Toggle visibility of Mode 0 (Difference between dates) specific widgets
	if (lblTo) {
		if (is_addsub) lblTo->hide();
		else lblTo->show();
	}
	if (mDateToPicker) {
		if (is_addsub) mDateToPicker->hide();
		else mDateToPicker->show();
	}
	if (mDateDiffResultDays) {
		if (is_addsub) mDateDiffResultDays->hide();
		else mDateDiffResultDays->show();
	}

	// Toggle visibility of Mode 1 (Add or subtract days) specific widgets
	if (mAddRadio) {
		if (is_addsub) mAddRadio->show();
		else mAddRadio->hide();
	}
	if (mSubRadio) {
		if (is_addsub) mSubRadio->show();
		else mSubRadio->hide();
	}
	if (lblYears) {
		if (is_addsub) lblYears->show();
		else lblYears->hide();
	}
	if (mYearsCombo) {
		if (is_addsub) mYearsCombo->show();
		else mYearsCombo->hide();
	}
	if (lblMonths) {
		if (is_addsub) lblMonths->show();
		else lblMonths->hide();
	}
	if (mMonthsCombo) {
		if (is_addsub) mMonthsCombo->show();
		else mMonthsCombo->hide();
	}
	if (lblDays) {
		if (is_addsub) lblDays->show();
		else lblDays->hide();
	}
	if (mDaysCombo) {
		if (is_addsub) mDaysCombo->show();
		else mDaysCombo->hide();
	}

	// Change labels depending on mode
	if (mDateDiffLabel) mDateDiffLabel->setText(is_addsub ? "Date" : "Difference");

	updateDateDiffResult();
}

void Calculator::slotDateFromChanged(const TQDate &date)
{
	(void)date;
	updateDateDiffResult();
}

void Calculator::slotDateToChanged(const TQDate &date)
{
	(void)date;
	updateDateDiffResult();
}

void Calculator::slotDateInputChanged(void)
{
	updateDateDiffResult();
}

void Calculator::updateDateDiffResult(void)
{
	if (!mDateFromPicker || !mDateToPicker) return;
	if (!mDateDiffResult || !mDateDiffResultDays) return;
	if (!mDateModeCombo) return;

	if (mDateModeCombo->currentItem() == 1) {
		// "Add or subtract days" Mode
		TQDate from = mDateFromPicker->date();
		if (!from.isValid()) {
			mDateDiffResult->setText(tr_str("Invalid date"));
			mDateDiffResultDays->setText("");
			return;
		}

		int years = mYearsCombo ? mYearsCombo->value() : 0;
		int months = mMonthsCombo ? mMonthsCombo->value() : 0;
		int days = mDaysCombo ? mDaysCombo->value() : 0;

		TQDate resultDate = from;
		if (mAddRadio && mAddRadio->isChecked()) {
			resultDate = resultDate.addYears(years);
			resultDate = resultDate.addMonths(months);
			resultDate = resultDate.addDays(days);
		} else {
			// Subtract: days, then months, then years
			resultDate = resultDate.addDays(-days);
			resultDate = resultDate.addMonths(-months);
			resultDate = resultDate.addYears(-years);
		}

		if (!resultDate.isValid()) {
			mDateDiffResult->setText(tr_str("Date out of range"));
			mDateDiffResultDays->setText("");
			return;
		}

		// Format long date localized safely without relying on uninitialized TDEGlobal::locale()
		TQString dateStr;
		CalcLang activeLang = Translation::lang();
		bool isFrench = (activeLang == LangFrench);
		if (isFrench) {
			dateStr = TQDate::longDayName(resultDate.dayOfWeek()) + " " +
			          TQString::number(resultDate.day()) + " " +
			          TQDate::longMonthName(resultDate.month()) + " " +
			          TQString::number(resultDate.year());
		} else {
			dateStr = TQDate::longDayName(resultDate.dayOfWeek()) + ", " +
			          TQDate::longMonthName(resultDate.month()) + " " +
			          TQString::number(resultDate.day()) + ", " +
			          TQString::number(resultDate.year());
		}
		mDateDiffResult->setText(dateStr);
		mDateDiffResultDays->setText("");
		return;
	}

	// Mode 0: "Difference between dates" Mode
	TQDate from = mDateFromPicker->date();
	TQDate to = mDateToPicker->date();

	if (!from.isValid() || !to.isValid()) {
		mDateDiffResult->setText(tr_str("Invalid date"));
		mDateDiffResultDays->setText("");
		return;
	}

	int totalDays = from.daysTo(to);
	if (totalDays < 0) totalDays = -totalDays;

	if (totalDays == 0) {
		mDateDiffResult->setText(tr_str("Same dates"));
		mDateDiffResultDays->setText("");
		return;
	}

	// Compute years, months, weeks, days breakdown
	// Use calendar arithmetic like the MS Calculator:
	// Walk from 'from' to 'to', advancing by years, then months, then counting remaining days.
	TQDate earlier = (from < to) ? from : to;
	TQDate later = (from < to) ? to : from;

	int years = 0, months = 0, weeks = 0, days = 0;

	// Count full years
	TQDate temp = earlier.addYears(1);
	while (temp <= later) {
		years++;
		earlier = earlier.addYears(1);
		temp = earlier.addYears(1);
	}

	// Count full months
	temp = earlier.addMonths(1);
	while (temp <= later) {
		months++;
		earlier = earlier.addMonths(1);
		temp = earlier.addMonths(1);
	}

	// Remaining days
	int remainDays = earlier.daysTo(later);
	weeks = remainDays / 7;
	days = remainDays % 7;

	// Build the human-readable string (matching MS: "Y years, M months, W weeks, D days")
	TQString result;
	bool needSep = false;

	if (years > 0) {
		result += TQString::number(years) + " " + (years == 1 ? tr_str("year") : tr_str("years"));
		needSep = true;
	}
	if (months > 0) {
		if (needSep) result += "; ";
		result += TQString::number(months) + " " + (months == 1 ? tr_str("month") : tr_str("months"));
		needSep = true;
	}
	if (weeks > 0) {
		if (needSep) result += "; ";
		result += TQString::number(weeks) + " " + (weeks == 1 ? tr_str("week") : tr_str("weeks"));
		needSep = true;
	}
	if (days > 0) {
		if (needSep) result += "; ";
		result += TQString::number(days) + " " + (days == 1 ? tr_str("day") : tr_str("days"));
	}

	// Edge case: if years/months/weeks are all 0, the result is just the days
	if (result.isEmpty()) {
		result = TQString::number(totalDays) + " " + (totalDays == 1 ? tr_str("day") : tr_str("days"));
	}

	mDateDiffResult->setText(result);

	// Show total days below (only if the breakdown wasn't just days)
	if (years > 0 || months > 0 || weeks > 0) {
		mDateDiffResultDays->setText(TQString::number(totalDays) + " " + (totalDays == 1 ? tr_str("day") : tr_str("days")));
	} else {
		mDateDiffResultDays->setText("");
	}
}

// --- ProgMemoryRowWidget Implementation ---

ProgMemoryRowWidget::ProgMemoryRowWidget(const KNumber &val, int index, Calculator *calc, TQWidget *parent)
	: TQWidget(parent), m_val(val), m_index(index), m_calc(calc)
{
	TQColor panelBg = m_calc->panelBackgroundColor();
	setBackgroundMode(TQt::PaletteBackground);
	setPaletteBackgroundColor(panelBg);
	setFixedHeight(74);

	TQVBoxLayout *mainLay = new TQVBoxLayout(this, 8, 2);

	// Value label
	m_lblValue = new TQLabel(m_calc->formatMemoryValue(m_val), this);
	m_lblValue->setBackgroundMode(TQt::PaletteBackground);
	m_lblValue->setPaletteBackgroundColor(panelBg);
	m_lblValue->setPaletteForegroundColor(m_calc->panelForegroundColor());
	m_lblValue->setAlignment(TQt::AlignRight | TQt::AlignVCenter);
	TQFont f = m_lblValue->font();
	f.setPointSize(12);
	f.setBold(true);
	m_lblValue->setFont(f);
	mainLay->addWidget(m_lblValue);

	// Button container (always shown, fixed height)
	m_btnContainer = new TQWidget(this);
	m_btnContainer->setBackgroundMode(TQt::PaletteBackground);
	m_btnContainer->setPaletteBackgroundColor(panelBg);
	m_btnContainer->setFixedHeight(26);
	TQHBoxLayout *btnLay = new TQHBoxLayout(m_btnContainer, 0, 4);
	btnLay->addStretch(1);

	m_btnMC = new CalcButton("MC", m_btnContainer);
	m_btnMC->setButtonType(CalcButton::TypeMemory);
	m_btnMC->setFlat(true);
	m_btnMC->setFixedWidth(36);
	m_btnMC->setFixedHeight(22);
	TQToolTip::add(m_btnMC, tr_str("Clear from memory"));
	btnLay->addWidget(m_btnMC);
	connect(m_btnMC, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotMCClicked()));

	m_btnMPlus = new CalcButton("M+", m_btnContainer);
	m_btnMPlus->setButtonType(CalcButton::TypeMemory);
	m_btnMPlus->setFlat(true);
	m_btnMPlus->setFixedWidth(36);
	m_btnMPlus->setFixedHeight(22);
	TQToolTip::add(m_btnMPlus, tr_str("Add display value to this memory entry"));
	btnLay->addWidget(m_btnMPlus);
	connect(m_btnMPlus, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotMPlusClicked()));

	m_btnMMinus = new CalcButton("M-", m_btnContainer);
	m_btnMMinus->setButtonType(CalcButton::TypeMemory);
	m_btnMMinus->setFlat(true);
	m_btnMMinus->setFixedWidth(36);
	m_btnMMinus->setFixedHeight(22);
	TQToolTip::add(m_btnMMinus, tr_str("Subtract display value from this memory entry"));
	btnLay->addWidget(m_btnMMinus);
	connect(m_btnMMinus, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotMMinusClicked()));

	mainLay->addWidget(m_btnContainer);

	// Hide the buttons initially
	m_btnMC->hide();
	m_btnMPlus->hide();
	m_btnMMinus->hide();
}

void ProgMemoryRowWidget::enterEvent(TQEvent *)
{
	m_btnMC->show();
	m_btnMPlus->show();
	m_btnMMinus->show();

	TQColor panelBg = m_calc->panelBackgroundColor();
	int val = (panelBg.red() * 299 + panelBg.green() * 587 + panelBg.blue() * 114) / 1000;
	TQColor hoverColor = (val > 128) ? panelBg.dark(112) : panelBg.light(125);
	if (val < 10) {
		hoverColor = TQColor(35, 35, 35);
	}

	setPaletteBackgroundColor(hoverColor);
	m_lblValue->setPaletteBackgroundColor(hoverColor);
	m_btnContainer->setPaletteBackgroundColor(hoverColor);
	m_lblValue->update();
	m_btnContainer->update();
	update();
}

void ProgMemoryRowWidget::leaveEvent(TQEvent *)
{
	m_btnMC->hide();
	m_btnMPlus->hide();
	m_btnMMinus->hide();

	TQColor normalColor = m_calc->panelBackgroundColor();
	setPaletteBackgroundColor(normalColor);
	m_lblValue->setPaletteBackgroundColor(normalColor);
	m_btnContainer->setPaletteBackgroundColor(normalColor);
	m_lblValue->update();
	m_btnContainer->update();
	update();
}

void ProgMemoryRowWidget::paintEvent(TQPaintEvent *)
{
	TQPainter p(this);
	p.fillRect(rect(), paletteBackgroundColor());
	
	TQColor panelBg = m_calc->panelBackgroundColor();
	int val = (panelBg.red() * 299 + panelBg.green() * 587 + panelBg.blue() * 114) / 1000;
	TQColor lineColor = (val > 128) ? panelBg.dark(110) : panelBg.light(115);
	if (val < 10) {
		lineColor = TQColor(40, 40, 40);
	}
	
	p.setPen(lineColor);
	p.drawLine(0, height() - 1, width(), height() - 1);
}

void ProgMemoryRowWidget::updateColors()
{
	TQColor normalColor = m_calc->panelBackgroundColor();
	setPaletteBackgroundColor(normalColor);
	m_lblValue->setPaletteBackgroundColor(normalColor);
	m_lblValue->setPaletteForegroundColor(m_calc->panelForegroundColor());
	m_btnContainer->setPaletteBackgroundColor(normalColor);
	update();
}

void ProgMemoryRowWidget::mousePressEvent(TQMouseEvent *)
{
	m_calc->recallMemoryEntry(m_val);
}

void ProgMemoryRowWidget::updateRowSizes(double scale)
{
	int row_height = static_cast<int>(74.0 * scale);
	if (row_height < 74) row_height = 74;
	setFixedHeight(row_height);

	int font_size = static_cast<int>(12.0 * scale);
	if (font_size < 12) font_size = 12;
	TQFont f = m_lblValue->font();
	f.setPointSize(font_size);
	m_lblValue->setFont(f);

	int btn_w = static_cast<int>(36.0 * scale);
	int btn_h = static_cast<int>(22.0 * scale);
	if (btn_w < 36) btn_w = 36;
	if (btn_h < 22) btn_h = 22;

	m_btnMC->setFixedSize(btn_w, btn_h);
	m_btnMPlus->setFixedSize(btn_w, btn_h);
	m_btnMMinus->setFixedSize(btn_w, btn_h);
	m_btnContainer->setFixedHeight(btn_h + 4);

	TQVBoxLayout *mainLay = dynamic_cast<TQVBoxLayout*>(layout());
	if (mainLay) {
		int margin = static_cast<int>(8.0 * scale);
		if (margin < 8) margin = 8;
		int spacing = static_cast<int>(2.0 * scale);
		if (spacing < 2) spacing = 2;
		mainLay->setMargin(margin);
		mainLay->setSpacing(spacing);
	}
}

void ProgMemoryRowWidget::slotMCClicked()
{
	m_calc->deleteProgMemoryEntry(m_index);
}

void ProgMemoryRowWidget::slotMPlusClicked()
{
	m_calc->addProgMemoryEntry(m_index, m_calc->currentDisplayValue());
}

void ProgMemoryRowWidget::slotMMinusClicked()
{
	m_calc->subtractProgMemoryEntry(m_index, m_calc->currentDisplayValue());
}

// --- Calculator Programmer Memory System Helper Methods ---

void Calculator::deleteProgMemoryEntry(int index)
{
	if (index >= 0 && index < (int)m_programmerMemoryStack.count()) {
		m_programmerMemoryStack.remove(m_programmerMemoryStack.at(index));
		updateProgMemoryViews();
	}
}

void Calculator::addProgMemoryEntry(int index, const KNumber &val)
{
	if (index >= 0 && index < (int)m_programmerMemoryStack.count()) {
		TQValueList<KNumber>::Iterator it = m_programmerMemoryStack.at(index);
		if (it != m_programmerMemoryStack.end()) {
			*it += val;
			updateProgMemoryViews();
		}
	}
}

void Calculator::subtractProgMemoryEntry(int index, const KNumber &val)
{
	if (index >= 0 && index < (int)m_programmerMemoryStack.count()) {
		TQValueList<KNumber>::Iterator it = m_programmerMemoryStack.at(index);
		if (it != m_programmerMemoryStack.end()) {
			*it -= val;
			updateProgMemoryViews();
		}
	}
}

KNumber Calculator::currentDisplayValue() const
{
	return calc_display->getAmount();
}

TQString Calculator::formatMemoryValue(const KNumber &new_amount)
{
	int num_base = calc_display->base();
	if (num_base != 10 && new_amount.type() != KNumber::SpecialType)
	{
		KNumber display_amount = new_amount.integerPart();
		unsigned long long int tmp_workaround = static_cast<unsigned long long int>(display_amount);
		if (_word_size == 1) tmp_workaround &= 0xFFFFFFFFull;
		else if (_word_size == 2) tmp_workaround &= 0xFFFFull;
		else if (_word_size == 3) tmp_workaround &= 0xFFull;

		TQString raw_str = TQString::number(tmp_workaround, num_base).upper();
		if (num_base == 2) {
			TQString grouped;
			int len = raw_str.length();
			for (int i = 0; i < len; ++i) {
				grouped += raw_str[i];
				if ((len - 1 - i) % 4 == 0 && i != len - 1) {
					grouped += " ";
				}
			}
			return grouped;
		}
		return raw_str;
	}
	else
	{
		KNumber display_amount = new_amount;
		if (_word_size > 0 && new_amount.type() != KNumber::SpecialType) {
			long long val = static_cast<signed long int>(new_amount);
			if (_word_size == 1) val = static_cast<int32_t>(val);
			else if (_word_size == 2) val = static_cast<int16_t>(val);
			else if (_word_size == 3) val = static_cast<int8_t>(val);
			display_amount = KNumber(static_cast<signed long int>(val));
		}
		TQString display_str = display_amount.toTQString(9, -1);
		
		if (display_amount.type() == KNumber::IntegerType) {
			TQString formatted;
			TQString sign;
			TQString abs_str = display_str;
			if (display_str.startsWith("-")) {
				sign = "-";
				abs_str = display_str.mid(1);
			}
			int len = abs_str.length();
			for (int i = 0; i < len; ++i) {
				formatted += abs_str[i];
				if ((len - 1 - i) % 3 == 0 && i != len - 1) {
					formatted += " ";
				}
			}
			return sign + formatted;
		}
		return display_str;
	}
}

TQWidget* Calculator::createMemoryFrame(TQWidget *parent, TQScrollView *&outScroll)
{
	TQWidget *frame = new TQWidget(parent);
	frame->setPaletteBackgroundColor(TQColor(245, 245, 245));

	TQVBoxLayout *layout = new TQVBoxLayout(frame, 0, 0);

	outScroll = new TQScrollView(frame);
	outScroll->setFrameStyle(TQFrame::NoFrame);
	outScroll->setHScrollBarMode(TQScrollView::AlwaysOff);
	outScroll->setVScrollBarMode(TQScrollView::Auto);
	outScroll->setResizePolicy(TQScrollView::AutoOneFit);
	outScroll->setPaletteBackgroundColor(TQColor(245, 245, 245));
	outScroll->viewport()->setBackgroundMode(TQt::PaletteBackground);
	outScroll->viewport()->setPaletteBackgroundColor(TQColor(245, 245, 245));
	outScroll->viewport()->installEventFilter(this);
	layout->addWidget(outScroll, 1);

	mProgMemoryContainer = new TQWidget(outScroll->viewport(), "MemoryContainer");
	mProgMemoryContainer->setBackgroundMode(TQt::PaletteBackground);
	mProgMemoryContainer->setPaletteBackgroundColor(TQColor(245, 245, 245));
	new TQVBoxLayout(mProgMemoryContainer, 0, 0);
	outScroll->addChild(mProgMemoryContainer);

	mProgMemoryBottomBar = new TQWidget(frame);
	mProgMemoryBottomBar->setPaletteBackgroundColor(TQColor(245, 245, 245));
	TQHBoxLayout *botLayout = new TQHBoxLayout(mProgMemoryBottomBar, 4, 4);
	botLayout->addStretch(1);

	mProgMemoryDeleteButton = new CalcButton(mProgMemoryBottomBar, "pbProgMemoryDelete");
	mProgMemoryDeleteButton->setFlat(true);
	mProgMemoryDeleteButton->setButtonType(CalcButton::TypeOther);
	mProgMemoryDeleteButton->setBackgroundColor(TQColor(245, 245, 245));
	mProgMemoryDeleteButton->setPaletteBackgroundColor(TQColor(245, 245, 245));
	applyIconToButton(mProgMemoryDeleteButton, trash_png, trash_png_len, tr_str("Clear all memory"));
	mProgMemoryDeleteButton->setFixedWidth(32);
	mProgMemoryDeleteButton->setFixedHeight(32);
	botLayout->addWidget(mProgMemoryDeleteButton);

	connect(mProgMemoryDeleteButton, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotProgMCclicked()));

	layout->addWidget(mProgMemoryBottomBar, 0);

	return frame;
}

void Calculator::rebuildProgMemoryFrame()
{
	if (!mProgMemoryFrameScroll || !mProgMemoryContainer) return;

	// Delete all child widgets of container
	const TQObjectList *list = mProgMemoryContainer->children();
	if (list) {
		TQObjectList copy = *list;
		TQObjectListIt it(copy);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			if (obj->isWidgetType()) {
				delete obj;
			}
		}
	}
	delete mProgMemoryContainer->layout();

	TQVBoxLayout *lay = new TQVBoxLayout(mProgMemoryContainer, 0, 0);

	int index = 0;
	for (TQValueList<KNumber>::ConstIterator it = m_programmerMemoryStack.begin(); it != m_programmerMemoryStack.end(); ++it) {
		ProgMemoryRowWidget *row = new ProgMemoryRowWidget(*it, index, this, mProgMemoryContainer);
		row->show();
		lay->addWidget(row);
		index++;
	}
	lay->addStretch(1);

	updateProgMemorySizes();
}

void Calculator::updateProgMemoryViews()
{
	if (m_programmerMemoryStack.isEmpty()) {
		calc_display->setStatusText(3, TQString());
		if (btnProgM) btnProgM->setEnabled(false);
		if (m_programmerMemoryFrameVisible) {
			slotToggleProgMemoryFrame();
		}
	} else {
		calc_display->setStatusText(3, "M");
		if (btnProgM) btnProgM->setEnabled(true);
	}

	rebuildProgMemoryFrame();
}

void Calculator::updateProgMemorySizes()
{
	if (!mProgMemoryContainer || !mProgMemoryFrameScroll) return;

	double scale = static_cast<double>(height()) / 500.0;
	if (scale < 1.0) scale = 1.0;

	if (mProgMemoryDeleteButton) {
		int trash_sz = static_cast<int>(32.0 * scale);
		if (trash_sz < 32) trash_sz = 32;
		mProgMemoryDeleteButton->setFixedSize(trash_sz, trash_sz);
	}

	const TQObjectList *childList = mProgMemoryContainer->children();
	if (childList) {
		TQObjectListIt it(*childList);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			ProgMemoryRowWidget *row = dynamic_cast<ProgMemoryRowWidget*>(obj);
			if (row) {
				row->updateRowSizes(scale);
			}
		}
	}

	mProgMemoryContainer->updateGeometry();
	int w = mProgMemoryFrameScroll->visibleWidth();
	mProgMemoryContainer->resize(w, mProgMemoryContainer->sizeHint().height());
	mProgMemoryFrameScroll->resizeContents(w, mProgMemoryContainer->height());
}

void Calculator::setProgrammerGridVisible(bool visible)
{
	if (visible) {
		if (m_programmer_bitboard_view) {
			slotBitboardViewClicked();
		} else {
			slotKeypadViewClicked();
		}
	} else {
		TQPtrListIterator<TQWidget> it(mProgKeypadWidgets);
		TQWidget *w;
		while ((w = it.current()) != 0) {
			++it;
			w->hide();
		}
		if (mBitboardPage) mBitboardPage->hide();
	}
}

void Calculator::slotProgMemStoreclicked()
{
	EnterEqual();
	m_programmerMemoryStack.prepend(calc_display->getAmount());
	updateProgMemoryViews();
}

void Calculator::slotToggleProgMemoryFrame()
{
	m_programmerMemoryFrameVisible = !m_programmerMemoryFrameVisible;

	if (m_programmerMemoryFrameVisible) {
		if (btnProgM) btnProgM->setText(TQString::fromUtf8("M\xe2\x96\xb2"));
	} else {
		if (btnProgM) btnProgM->setText(TQString::fromUtf8("M\xe2\x96\xbc"));
	}

	if (_calc_mode == ModeProgrammer) {
		if (m_programmerMemoryFrameVisible) {
			setProgrammerGridVisible(false);
			mProgrammerMemoryFrame->show();
			rebuildProgMemoryFrame();
		} else {
			mProgrammerMemoryFrame->hide();
			setProgrammerGridVisible(true);
		}
	}
}

void Calculator::slotProgMCclicked(void)
{
	m_programmerMemoryStack.clear();
	updateProgMemoryViews();
}

void Calculator::setupConverterPage(TQWidget *parent)
{
	mConverterPage = new TQWidget(parent);
	mConverterPage->setBackgroundMode(TQt::PaletteBackground);

	TQVBoxLayout *mainLay = new TQVBoxLayout(mConverterPage, 16, 12);

	// TOP PANEL (Slots, combos)
	m_converterTopPanel = new TQWidget(mConverterPage);
	TQVBoxLayout *topLay = new TQVBoxLayout(m_converterTopPanel, 0, 0);

	// Slot 1
	m_lblVal1 = new TQLabel("0", m_converterTopPanel);
	m_lblVal1->setAlignment(AlignLeft | AlignVCenter);
	topLay->addWidget(m_lblVal1);

	m_lblVal1_lcd = new TQtLcdWidget(m_converterTopPanel);
	m_lblVal1_lcd->setRow(1);
	m_lblVal1_lcd->setColumn(24);
	m_lblVal1_lcd->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);
	m_lblVal1_lcd->hide();
	m_lblVal1_lcd->clear();
	m_lblVal1_lcd->string(TQString("0").rightJustify(m_lblVal1_lcd->currentColumn(), ' ', true));
	topLay->addWidget(m_lblVal1_lcd);

	m_comboUnit1 = new TQComboBox(false, m_converterTopPanel);
	m_comboUnit1->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Fixed);
	topLay->addWidget(m_comboUnit1);

	// Spacer between slots
	topLay->addSpacing(16);

	// Slot 2
	m_lblVal2 = new TQLabel("0", m_converterTopPanel);
	m_lblVal2->setAlignment(AlignLeft | AlignVCenter);
	topLay->addWidget(m_lblVal2);

	m_lblVal2_lcd = new TQtLcdWidget(m_converterTopPanel);
	m_lblVal2_lcd->setRow(1);
	m_lblVal2_lcd->setColumn(24);
	m_lblVal2_lcd->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Preferred);
	m_lblVal2_lcd->hide();
	m_lblVal2_lcd->clear();
	m_lblVal2_lcd->string(TQString("0").rightJustify(m_lblVal2_lcd->currentColumn(), ' ', true));
	topLay->addWidget(m_lblVal2_lcd);

	m_comboUnit2 = new TQComboBox(false, m_converterTopPanel);
	m_comboUnit2->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Fixed);
	topLay->addWidget(m_comboUnit2);

	mainLay->addWidget(m_converterTopPanel);

	mainLay->addSpacing(8);

	// BOTTOM PANEL (Keypad layout)
	TQGridLayout *keypadLay = new TQGridLayout(5, 3, 2);
	keypadLay->setSpacing(2);

	keypadLay->setColStretch(0, 1);
	keypadLay->setColStretch(1, 1);
	keypadLay->setColStretch(2, 1);
	keypadLay->setRowStretch(0, 1);
	keypadLay->setRowStretch(1, 1);
	keypadLay->setRowStretch(2, 1);
	keypadLay->setRowStretch(3, 1);
	keypadLay->setRowStretch(4, 1);

	// Row 0: CE, Backspace
	m_btnConvCE = new CalcButton("CE", mConverterPage, "ConvCE-Button");
	m_btnConvCE->setButtonType(CalcButton::TypeOperator);
	m_btnConvCE->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	keypadLay->addWidget(m_btnConvCE, 0, 1);

	m_btnConvBS = new CalcButton(mConverterPage, "ConvBS-Button");
	m_btnConvBS->setButtonType(CalcButton::TypeOperator);
	m_btnConvBS->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	applyIconToButton(m_btnConvBS, backspace_png, backspace_png_len, "Backspace");
	keypadLay->addWidget(m_btnConvBS, 0, 2);

	// Rows 1-3: Digits 1-9
	for (int digit = 1; digit <= 9; ++digit) {
		m_btnConvDigits[digit] = new CalcButton(TQString::number(digit), mConverterPage);
		m_btnConvDigits[digit]->setButtonType(CalcButton::TypeDigit);
		m_btnConvDigits[digit]->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
		
		int r = 3 - (digit - 1) / 3; // digit 1 is row 3, digit 9 is row 1
		int c = (digit - 1) % 3;
		keypadLay->addWidget(m_btnConvDigits[digit], r, c);
	}

	// Row 4: 0, Comma
	m_btnConvDigits[0] = new CalcButton("0", mConverterPage);
	m_btnConvDigits[0]->setButtonType(CalcButton::TypeDigit);
	m_btnConvDigits[0]->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	keypadLay->addWidget(m_btnConvDigits[0], 4, 1);

	m_btnConvComma = new CalcButton(",", mConverterPage);
	m_btnConvComma->setButtonType(CalcButton::TypeOperator);
	m_btnConvComma->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Expanding);
	keypadLay->addWidget(m_btnConvComma, 4, 2);

	mainLay->addLayout(keypadLay, 10);

	// Equivalence section (below keypad, reserving space)
	mainLay->addSpacing(4);
	m_lblEquivTitle = new TQLabel(" ", mConverterPage);
	m_lblEquivTitle->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Fixed);
	mainLay->addWidget(m_lblEquivTitle);

	m_lblEquiv = new TQLabel(" ", mConverterPage);
	m_lblEquiv->setAlignment(AlignLeft | AlignTop | WordBreak);
	m_lblEquiv->setSizePolicy(TQSizePolicy::Expanding, TQSizePolicy::Fixed);
	mainLay->addWidget(m_lblEquiv);

	// Connects
	connect(m_comboUnit1, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotConvUnit1Changed(int)));
	connect(m_comboUnit2, TQ_SIGNAL(activated(int)), this, TQ_SLOT(slotConvUnit2Changed(int)));
	
	connect(m_btnConvCE, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotConvCEClicked()));
	connect(m_btnConvBS, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotConvBSClicked()));
	connect(m_btnConvComma, TQ_SIGNAL(clicked()), this, TQ_SLOT(slotConvCommaClicked()));
	for (int i = 0; i < 10; ++i) {
		connect(m_btnConvDigits[i], TQ_SIGNAL(clicked()), this, TQ_SLOT(slotConvDigitClicked()));
	}

	// Install event filters for click detection on labels
	m_lblVal1->installEventFilter(this);
	m_lblVal2->installEventFilter(this);
	m_lblVal1_lcd->installEventFilter(this);
	m_lblVal2_lcd->installEventFilter(this);

	// Register comparison icons into default mime source factory for inline HTML display
	updateConversionMimeIcons();

	// Setup default values based on current mode
	int idx = _calc_mode - ModeVolume;
	if (idx >= 0 && idx < 13) {
		m_convActiveSlot = m_convSelectedActiveSlot[idx];
		m_convInput1 = m_convSelectedInput1[idx];
		m_convInput2 = m_convSelectedInput2[idx];
		m_convUnit1 = m_convSelectedUnit1[idx];
		m_convUnit2 = m_convSelectedUnit2[idx];
	} else {
		m_convActiveSlot = 1;
		m_convInput1 = "0";
		m_convInput2 = "0";
		m_convUnit1 = 0;
		m_convUnit2 = 1;
	}

	populateConverterUnits(_calc_mode);

	recalculateConversion();

	// Hide initially
	mConverterPage->hide();
}

void Calculator::updateConverterSizes()
{
	if (!mConverterPage || !mConverterPage->isVisible()) return;

	int h = height();
	if (h < 50) return;

	TQFont baseFont;
	if (m_displayType == 0) {
		baseFont = TQFont("Segoe Calc");
	} else if (m_displayType == 2) {
		baseFont = m_displayFont;
	} else if (m_displayType == 4) {
		baseFont = TQFont("Calculator");
	} else if (m_displayType == 5) {
		baseFont = TQFont("Computo Monospace");
	} else if (m_displayType == 6) {
		baseFont = TQFont("Digital Counter 7");
	} else if (m_displayType == 7) {
		baseFont = TQFont("Pocket Calculator");
	} else if (m_displayType == 8) {
		baseFont = TQFont("ClassWiz Math CW");
	} else {
		baseFont = font();
	}

	TQFont activeFont = baseFont;
	activeFont.setPixelSize(h * 0.065);
	activeFont.setBold(true);

	TQFont inactiveFont = baseFont;
	inactiveFont.setPixelSize(h * 0.042);
	inactiveFont.setBold(false);

	TQFont comboFont = font();
	comboFont.setPixelSize(h * 0.028);
	comboFont.setBold(false);

	TQFont equivTitleFont = font();
	equivTitleFont.setPixelSize(h * 0.022);
	equivTitleFont.setBold(false);

	TQFont equivFont = font();
	equivFont.setPixelSize(h * 0.024);
	equivFont.setBold(false);

	int valHeight = h * 0.075;

	if (m_displayType == 3) {
		if (m_lblVal1) m_lblVal1->hide();
		if (m_lblVal2) m_lblVal2->hide();
		if (m_lblVal1_lcd) {
			m_lblVal1_lcd->show();
			m_lblVal1_lcd->setFixedHeight(valHeight);
		}
		if (m_lblVal2_lcd) {
			m_lblVal2_lcd->show();
			m_lblVal2_lcd->setFixedHeight(valHeight);
		}

		if (m_lblVal1_lcd && m_lblVal2_lcd) {
			TQColor bgCol = m_colorDisplayBg;
			TQColor borderC;
			TQColor activeBg = m_colorDisplayBg;
			TQColor inactiveBg;
			TQColor activeFg = m_colorDisplayFg;
			TQColor inactiveFg;

			if (m_invertIcons) {
				int r = bgCol.red() + 60;
				int g = bgCol.green() + 60;
				int b = bgCol.blue() + 60;
				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;
				borderC = TQColor(r, g, b);

				r = bgCol.red() + 35;
				g = bgCol.green() + 35;
				b = bgCol.blue() + 35;
				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;
				inactiveBg = TQColor(r, g, b);
			} else {
				int r = bgCol.red() - 60;
				int g = bgCol.green() - 60;
				int b = bgCol.blue() - 60;
				if (r < 0) r = 0;
				if (g < 0) g = 0;
				if (b < 0) b = 0;
				borderC = TQColor(r, g, b);

				r = bgCol.red() - 30;
				g = bgCol.green() - 30;
				b = bgCol.blue() - 30;
				if (r < 0) r = 0;
				if (g < 0) g = 0;
				if (b < 0) b = 0;
				inactiveBg = TQColor(r, g, b);
			}

			if (m_convActiveSlot == 1) {
				m_lblVal1_lcd->setColorBackground1(activeBg);
				m_lblVal1_lcd->setColorBackground2(activeBg);
				m_lblVal1_lcd->setColorPixel(activeFg);
				m_lblVal1_lcd->setBorder(true, borderC);

				m_lblVal2_lcd->setColorBackground1(inactiveBg);
				m_lblVal2_lcd->setColorBackground2(inactiveBg);
				m_lblVal2_lcd->setColorPixel(activeFg);
				m_lblVal2_lcd->setBorder(false);
			} else {
				m_lblVal1_lcd->setColorBackground1(inactiveBg);
				m_lblVal1_lcd->setColorBackground2(inactiveBg);
				m_lblVal1_lcd->setColorPixel(activeFg);
				m_lblVal1_lcd->setBorder(false);

				m_lblVal2_lcd->setColorBackground1(activeBg);
				m_lblVal2_lcd->setColorBackground2(activeBg);
				m_lblVal2_lcd->setColorPixel(activeFg);
				m_lblVal2_lcd->setBorder(true, borderC);
			}
		}
	} else {
		if (m_lblVal1_lcd) m_lblVal1_lcd->hide();
		if (m_lblVal2_lcd) m_lblVal2_lcd->hide();
		if (m_lblVal1) {
			m_lblVal1->show();
			m_lblVal1->setFixedHeight(valHeight);
		}
		if (m_lblVal2) {
			m_lblVal2->show();
			m_lblVal2->setFixedHeight(valHeight);
		}

		if (m_lblVal1 && m_lblVal2) {
			TQColor activeBg = m_colorDisplayBg;
			TQColor inactiveBg;
			if (m_invertIcons) {
				int r = activeBg.red() + 35;
				int g = activeBg.green() + 35;
				int b = activeBg.blue() + 35;
				if (r > 255) r = 255;
				if (g > 255) g = 255;
				if (b > 255) b = 255;
				inactiveBg = TQColor(r, g, b);
			} else {
				int r = activeBg.red() - 30;
				int g = activeBg.green() - 30;
				int b = activeBg.blue() - 30;
				if (r < 0) r = 0;
				if (g < 0) g = 0;
				if (b < 0) b = 0;
				inactiveBg = TQColor(r, g, b);
			}

			// Blend the display foreground and display background for inactive text
			int r_fg = (m_colorDisplayFg.red() + activeBg.red()) / 2;
			int g_fg = (m_colorDisplayFg.green() + activeBg.green()) / 2;
			int b_fg = (m_colorDisplayFg.blue() + activeBg.blue()) / 2;
			TQColor inactiveFg(r_fg, g_fg, b_fg);

			if (m_convActiveSlot == 1) {
				m_lblVal1->setFont(activeFont);
				m_lblVal2->setFont(inactiveFont);
				m_lblVal1->setPaletteBackgroundColor(activeBg);
				m_lblVal1->setPaletteForegroundColor(m_colorDisplayFg);
				m_lblVal2->setPaletteBackgroundColor(inactiveBg);
				m_lblVal2->setPaletteForegroundColor(inactiveFg);
			} else {
				m_lblVal1->setFont(inactiveFont);
				m_lblVal2->setFont(activeFont);
				m_lblVal1->setPaletteBackgroundColor(inactiveBg);
				m_lblVal1->setPaletteForegroundColor(inactiveFg);
				m_lblVal2->setPaletteBackgroundColor(activeBg);
				m_lblVal2->setPaletteForegroundColor(m_colorDisplayFg);
			}
		}
	}

	m_comboUnit1->setFont(comboFont);
	m_comboUnit2->setFont(comboFont);

	// Find the labels and set their fonts and heights
	TQObjectList *lbls = mConverterPage->queryList("TQLabel");
	if (lbls) {
		TQObjectListIt it(*lbls);
		TQObject *obj;
		while ((obj = it.current()) != 0) {
			++it;
			TQLabel *lbl = (TQLabel*)obj;
			if (lbl == m_lblEquiv) {
				lbl->setFont(equivFont);
			} else if (lbl == m_lblEquivTitle) {
				lbl->setFont(equivTitleFont);
				lbl->setPaletteForegroundColor(TQColor(120, 120, 120));
			}
		}
		delete lbls;
	}

	// Find all CalcButton children of the converter page and scale them
	TQObjectList *btns = mConverterPage->queryList("CalcButton");
	if (btns) {
		TQObjectListIt it(*btns);
		TQObject *obj;
		TQFont btnFont = font();
		btnFont.setPixelSize(h * 0.040);
		btnFont.setBold(true);
		while ((obj = it.current()) != 0) {
			++it;
			CalcButton *btn = (CalcButton*)obj;
			btn->setFont(btnFont);
		}
		delete btns;
	}

	// Layout margins and spacings
	TQBoxLayout *mainLay = dynamic_cast<TQBoxLayout*>(mConverterPage->layout());
	if (mainLay) {
		int margin = 4; // ultra small margin to stretch all elements horizontally to the window edges
		int spacing = h * 0.010;
		if (spacing < 3) spacing = 3;
		if (spacing > 7) spacing = 7;

		mainLay->setMargin(margin);
		mainLay->setSpacing(spacing);
	}

	// Update converter keypad labels and enabled state
	if (_calc_mode == ModeRoman) {
		if (m_convActiveSlot == 2) {
			// Roman slot active: buttons 1-7 are I, V, X, L, C, D, M. 8,9,0 and comma disabled.
			const char *romanLabels[] = { "", "I", "V", "X", "L", "C", "D", "M", "", "" };
			for (int i = 0; i < 10; ++i) {
				if (i >= 1 && i <= 7) {
					m_btnConvDigits[i]->setText(romanLabels[i]);
					m_btnConvDigits[i]->setEnabled(true);
				} else {
					m_btnConvDigits[i]->setText("");
					m_btnConvDigits[i]->setEnabled(false);
				}
			}
			m_btnConvComma->setText("");
			m_btnConvComma->setEnabled(false);
		} else {
			// Arabic slot active: standard digits 0-9 enabled, comma disabled (only integers)
			for (int i = 0; i < 10; ++i) {
				m_btnConvDigits[i]->setText(TQString::number(i));
				m_btnConvDigits[i]->setEnabled(true);
			}
			m_btnConvComma->setText("");
			m_btnConvComma->setEnabled(false);
		}
	} else {
		// Non-Roman mode: standard digits 0-9 and comma enabled
		for (int i = 0; i < 10; ++i) {
			m_btnConvDigits[i]->setText(TQString::number(i));
			m_btnConvDigits[i]->setEnabled(true);
		}
		m_btnConvComma->setText(",");
		m_btnConvComma->setEnabled(true);
	}

	TQTimer::singleShot(0, this, TQ_SLOT(slotUpdateEquivGeometry()));
}

void Calculator::slotConvUnit1Changed(int index)
{
	m_convUnit1 = index;
	recalculateConversion();
}

void Calculator::slotConvUnit2Changed(int index)
{
	m_convUnit2 = index;
	recalculateConversion();
}

void Calculator::slotUpdateEquivGeometry()
{
	recalculateConversion();
}

void Calculator::slotConvDigitClicked()
{
	const TQObject *btn = sender();
	if (!btn) return;
	for (int i = 0; i < 10; ++i) {
		if (btn == m_btnConvDigits[i]) {
			handleConvDigit(i);
			return;
		}
	}
}

void Calculator::slotConvCommaClicked()
{
	handleConvComma();
}

void Calculator::slotConvCEClicked()
{
	handleConvCE();
}

void Calculator::slotConvBSClicked()
{
	handleConvBS();
}



void Calculator::handleConvDigit(int digit)
{
	TQString &input = (m_convActiveSlot == 1) ? m_convInput1 : m_convInput2;
	if (_calc_mode == ModeRoman && m_convActiveSlot == 2) {
		// Roman input Slot 2
		if (input == "0" || input == "N" || input == tr_str("Invalid") || input == tr_str("Out of range")) {
			input = "";
		}
		// Map digit: 1=I, 2=V, 3=X, 4=L, 5=C, 6=D, 7=M
		const char *romanLetters[] = { "", "I", "V", "X", "L", "C", "D", "M" };
		if (digit >= 1 && digit <= 7) {
			if (input.length() < 15) {
				input += romanLetters[digit];
			}
		}
	} else {
		// Standard digit input
		if (input == "0" || input == tr_str("Invalid") || input == tr_str("Out of range")) {
			input = TQString::number(digit);
		} else if (input == "-0") {
			input = "-" + TQString::number(digit);
		} else {
			// Limit to 15 significant digits
			TQString digitsOnly = input;
			digitsOnly.remove(',');
			digitsOnly.remove('-');
			if (digitsOnly.length() < 15) {
				input += TQString::number(digit);
			}
		}
	}

	if (m_convActiveSlot == 1) {
		setVal1Text(input);
	} else {
		setVal2Text(input);
	}

	recalculateConversion();
}

void Calculator::handleConvComma()
{
	TQString &input = (m_convActiveSlot == 1) ? m_convInput1 : m_convInput2;
	if (input.contains(',')) return;

	if (input.isEmpty()) {
		input = "0,";
	} else {
		input += ",";
	}

	if (m_convActiveSlot == 1) {
		setVal1Text(input);
	} else {
		setVal2Text(input);
	}
}

void Calculator::handleConvCE()
{
	if (m_convActiveSlot == 1) {
		m_convInput1 = "0";
		setVal1Text("0");
	} else {
		m_convInput2 = (_calc_mode == ModeRoman) ? "" : "0";
		setVal2Text(m_convInput2);
	}
	recalculateConversion();
}

void Calculator::handleConvBS()
{
	TQString &input = (m_convActiveSlot == 1) ? m_convInput1 : m_convInput2;
	if (_calc_mode == ModeRoman && m_convActiveSlot == 2) {
		if (input.length() <= 1 || input == tr_str("Invalid") || input == tr_str("Out of range")) {
			input = "";
		} else {
			input.truncate(input.length() - 1);
		}
	} else {
		if (input.length() <= 1 || (input.length() == 2 && input.startsWith("-")) || input == tr_str("Invalid") || input == tr_str("Out of range")) {
			input = "0";
		} else {
			input.truncate(input.length() - 1);
		}
	}

	if (m_convActiveSlot == 1) {
		setVal1Text(input);
	} else {
		setVal2Text(input);
	}

	recalculateConversion();
}



void Calculator::setActiveSlot(int slot)
{
	m_convActiveSlot = slot;
	updateConverterSizes();
}

double Calculator::getVolumeFactor(int index)
{
	switch (index) {
		case 0: return 0.001;          // Milliliter(s)
		case 1: return 0.001;          // Cubic centimeter(s)
		case 2: return 1.0;            // Liter(s)
		case 3: return 1000.0;         // Cubic meter(s)
		case 4: return 0.016387064;    // Cubic inch(es)
		case 5: return 28.316846592;   // Cubic foot/feet
		case 6: return 0.2365882365;   // Cup(s) (U.S.)
		case 7: return 0.473176473;    // Pint(s) (U.S.)
		case 8: return 0.946352946;    // Quart(s) (U.S.)
		case 9: return 3.785411784;    // Gallon(s) (U.S.)
		case 10: return 4.54609;       // Gallon(s) (Imp.)
		case 11: return 378.5411784;   // Bathtub(s)
		default: return 1.0;
	}
}

void Calculator::recalculateConversion()
{
	if (!m_lblVal1 || !m_lblVal2 || !m_lblEquiv) return;

	if (_calc_mode == ModeRoman) {
		if (m_convActiveSlot == 1) {
			// Arabic -> Roman
			double activeVal = parseVal(m_convInput1);
			int intVal = static_cast<int>(activeVal);
			TQString targetStr;
			if (activeVal < 0 || activeVal > 3999 || intVal != activeVal) {
				targetStr = tr_str("Out of range");
			} else {
				targetStr = arabicToRoman(intVal);
			}
			m_convInput2 = targetStr;
			setVal2Text(targetStr);
		} else {
			// Roman -> Arabic
			int intVal = romanToArabic(m_convInput2);
			TQString targetStr;
			if (intVal < 0) {
				targetStr = tr_str("Invalid");
			} else {
				targetStr = TQString::number(intVal);
			}
			m_convInput1 = targetStr;
			setVal1Text(targetStr);
		}

		m_lblEquivTitle->setText(" ");
		m_lblEquiv->setText(" ");
		m_lblEquiv->setFixedHeight(20);
		return;
	}

	TQString activeStr = (m_convActiveSlot == 1) ? m_convInput1 : m_convInput2;
	double activeVal = parseVal(activeStr);

	int srcUnit = (m_convActiveSlot == 1) ? m_convUnit1 : m_convUnit2;
	int dstUnit = (m_convActiveSlot == 1) ? m_convUnit2 : m_convUnit1;

	double valBase = 0.0;
	double targetVal = 0.0;

	if (_calc_mode == ModeTemp) {
		// Temperature conversion (0=Celsius, 1=Fahrenheit, 2=Kelvin)
		if (srcUnit == 0) { // Celsius -> Kelvin
			valBase = activeVal + 273.15;
		} else if (srcUnit == 1) { // Fahrenheit -> Kelvin
			valBase = (activeVal - 32.0) * 5.0 / 9.0 + 273.15;
		} else { // Kelvin -> Kelvin
			valBase = activeVal;
		}

		if (dstUnit == 0) { // Kelvin -> Celsius
			targetVal = valBase - 273.15;
		} else if (dstUnit == 1) { // Kelvin -> Fahrenheit
			targetVal = (valBase - 273.15) * 9.0 / 5.0 + 32.0;
		} else { // Kelvin -> Kelvin
			targetVal = valBase;
		}
	} else {
		double toBase = getConversionFactor(_calc_mode, srcUnit);
		double fromBase = getConversionFactor(_calc_mode, dstUnit);
		valBase = activeVal * toBase;
		targetVal = valBase / fromBase;
	}

	TQString targetStr = formatDouble(targetVal);

	if (m_convActiveSlot == 1) {
		m_convInput2 = targetStr;
		setVal2Text(targetStr);
	} else {
		m_convInput1 = targetStr;
		setVal1Text(targetStr);
	}

	int h = height();
	int iconSize = h * 0.045;
	if (iconSize < 20) iconSize = 20;
	if (iconSize > 32) iconSize = 32;

	TQString sz = TQString::number(iconSize);
	TQString dummy = "99 999,9999999";
	TQString equivText;
	TQString measureText;

	switch (_calc_mode) {
		case ModeVolume: {
			double cupVal = valBase / 0.2365882;
			double bathVal = valBase / 378.5411784;
			double poolVal = valBase / 3750000.0;
			TQString cupStr = formatDouble(cupVal);
			TQString bathStr = formatDouble(bathVal);
			TQString poolStr = formatDouble(poolVal);

			equivText = TQString("<nobr><img src=\"teacup\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + cupStr + "</b> " + tr_str("coffee cup(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"bathtube\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + bathStr + "</b> " + tr_str("bathtub(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"swimmingpool\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + poolStr + "</b> " + tr_str("swimming pool(s)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"teacup\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("coffee cup(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"bathtube\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("bathtub(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"swimmingpool\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("swimming pool(s)") + "</nobr>";
			break;
		}
		case ModeLength: {
			double handVal = valBase / 0.2;
			double horseVal = valBase / 2.4;
			double whaleVal = valBase / 30.0;
			TQString handStr = formatDouble(handVal);
			TQString horseStr = formatDouble(horseVal);
			TQString whaleStr = formatDouble(whaleVal);

			equivText = TQString("<nobr><img src=\"hand\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + handStr + "</b> " + tr_str("handspan(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"horse\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + horseStr + "</b> " + tr_str("horse length(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"whale\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + whaleStr + "</b> " + tr_str("blue whale(s)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"hand\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("handspan(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"horse\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("horse length(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"whale\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("blue whale(s)") + "</nobr>";
			break;
		}
		case ModeMass: {
			double bananaVal = valBase / 0.12;
			double elephantVal = valBase / 5000.0;
			double whaleVal = valBase / 130000.0;
			TQString bananaStr = formatDouble(bananaVal);
			TQString elephantStr = formatDouble(elephantVal);
			TQString whaleStr = formatDouble(whaleVal);

			equivText = TQString("<nobr><img src=\"banana\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + bananaStr + "</b> " + tr_str("banana(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"elephant\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + elephantStr + "</b> " + tr_str("elephant(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"whale\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + whaleStr + "</b> " + tr_str("blue whale(s)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"banana\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("banana(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"elephant\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("elephant(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"whale\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("blue whale(s)") + "</nobr>";
			break;
		}
		case ModeTemp: {
			double cVal = valBase - 273.15;
			double bathDiff = cVal - 40.0;
			TQString cStr = formatDouble(cVal);
			TQString bathStr = formatDouble(bathDiff);

			equivText = TQString("<nobr><img src=\"snowflake\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + cStr + "</b> " + tr_str("°C relative to water freezing (0°C)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"bathtube\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + bathStr + "</b> " + tr_str("°C relative to a warm bath (40°C)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"snowflake\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("°C relative to water freezing (0°C)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"bathtube\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("°C relative to a warm bath (40°C)") + "</nobr>";
			break;
		}
		case ModeEnergy: {
			double batteryVal = valBase / 10000.0;
			double cakeVal = valBase / 1000000.0;
			TQString batteryStr = formatDouble(batteryVal);
			TQString cakeStr = formatDouble(cakeVal);

			equivText = TQString("<nobr><img src=\"battery\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + batteryStr + "</b> " + tr_str("AA battery capacity") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"cakeslice\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + cakeStr + "</b> " + tr_str("slice(s) of cake") + "</nobr>";
			measureText = TQString("<nobr><img src=\"battery\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("AA battery capacity") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"cakeslice\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("slice(s) of cake") + "</nobr>";
			break;
		}
		case ModeArea: {
			double paperVal = valBase / 0.0625;
			double footballVal = valBase / 7140.0;
			double castleVal = valBase / 10000.0;
			TQString paperStr = formatDouble(paperVal);
			TQString footballStr = formatDouble(footballVal);
			TQString castleStr = formatDouble(castleVal);

			equivText = TQString("<nobr><img src=\"papersheet\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + paperStr + "</b> " + tr_str("A4 sheet(s) of paper") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"football\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + footballStr + "</b> " + tr_str("football field(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"castle\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + castleStr + "</b> " + tr_str("castle(s)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"papersheet\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("A4 sheet(s) of paper") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"football\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("football field(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"castle\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("castle(s)") + "</nobr>";
			break;
		}
		case ModeSpeed: {
			double turtleVal = valBase / 0.1;
			double horseVal = valBase / 15.0;
			double jetVal = valBase / 250.0;
			TQString turtleStr = formatDouble(turtleVal);
			TQString horseStr = formatDouble(horseVal);
			TQString jetStr = formatDouble(jetVal);

			equivText = TQString("<nobr><img src=\"turtle\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + turtleStr + "</b> " + tr_str("turtle speed(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"horse\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + horseStr + "</b> " + tr_str("horse speed(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"jet\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + jetStr + "</b> " + tr_str("passenger jet speed(s)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"turtle\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("turtle speed(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"horse\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("horse speed(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"jet\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("passenger jet speed(s)") + "</nobr>";
			break;
		}
		case ModeTime: {
			double blinkVal = valBase / 0.3;
			double matchVal = valBase / 5400.0;
			TQString blinkStr = formatDouble(blinkVal);
			TQString matchStr = formatDouble(matchVal);

			equivText = TQString("<nobr><img src=\"snowflake\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + blinkStr + "</b> " + tr_str("eye blink(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"football\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + matchStr + "</b> " + tr_str("football match(es)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"snowflake\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("eye blink(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"football\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("football match(es)") + "</nobr>";
			break;
		}
		case ModePower: {
			double horseVal = valBase / 745.7;
			double jetVal = valBase / 40000000.0;
			TQString horseStr = formatDouble(horseVal);
			TQString jetStr = formatDouble(jetVal);

			equivText = TQString("<nobr><img src=\"horse\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + horseStr + "</b> " + tr_str("horse(s) power") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"jet\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + jetStr + "</b> " + tr_str("passenger jet(s) power") + "</nobr>";
			measureText = TQString("<nobr><img src=\"horse\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("horse(s) power") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"jet\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("passenger jet(s) power") + "</nobr>";
			break;
		}
		case ModeData: {
			double paperVal = valBase / 2048.0;
			TQString paperStr = formatDouble(paperVal);

			equivText = TQString("<nobr><img src=\"papersheet\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + paperStr + "</b> " + tr_str("page(s) of text") + "</nobr>";
			measureText = TQString("<nobr><img src=\"papersheet\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("page(s) of text") + "</nobr>";
			break;
		}
		case ModePressure: {
			double snowVal = valBase / 1.0;
			double elephantVal = valBase / 100000.0;
			TQString snowStr = formatDouble(snowVal);
			TQString elephantStr = formatDouble(elephantVal);

			equivText = TQString("<nobr><img src=\"snowflake\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + snowStr + "</b> " + tr_str("snowflake pressure(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"elephant\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + elephantStr + "</b> " + tr_str("elephant standing pressure(s)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"snowflake\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("snowflake pressure(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"elephant\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("elephant standing pressure(s)") + "</nobr>";
			break;
		}
		case ModeAngle: {
			double rotVal = valBase / 6.283185307179586;
			double rightVal = valBase / 1.5707963267948966;
			TQString rotStr = formatDouble(rotVal);
			TQString rightStr = formatDouble(rightVal);

			equivText = TQString("<nobr><img src=\"football\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + rotStr + "</b> " + tr_str("full rotation(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"hand\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + rightStr + "</b> " + tr_str("right angle(s)") + "</nobr>";
			measureText = TQString("<nobr><img src=\"football\" width=\"") + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("full rotation(s)") + "</nobr> &nbsp;&nbsp;&nbsp; <nobr><img src=\"hand\" width=\"" + sz + "\" height=\"" + sz + "\" align=\"middle\" /> <b>" + dummy + "</b> " + tr_str("right angle(s)") + "</nobr>";
			break;
		}
		default:
			break;
	}

	int labelWidth = mConverterPage->width() - 8;
	if (labelWidth < 50) labelWidth = 50;

	TQSimpleRichText rt(measureText, m_lblEquiv->font(), "", 0, TQMimeSourceFactory::defaultFactory());
	rt.setWidth(labelWidth);
	int neededHeight = rt.height() + 4;
	m_lblEquiv->setFixedHeight(neededHeight);

	if (activeVal == 0.0) {
		m_lblEquivTitle->setText(" ");
		m_lblEquiv->setText(" ");
	} else {
		m_lblEquivTitle->setText(tr_str("About equal to"));
		m_lblEquiv->setText(equivText);
	}
}

TQString Calculator::formatDouble(double val)
{
	if (val == 0.0) return "0";

	TQString str;
	str.setNum(val, 'g', 8);

	str.replace('.', ',');

	if (str.contains('e') || str.contains('E')) {
		return str;
	}

	int commaIdx = str.find(',');
	TQString intPart = (commaIdx != -1) ? str.left(commaIdx) : str;
	TQString fracPart = (commaIdx != -1) ? str.mid(commaIdx) : "";

	TQString formattedInt;
	int len = intPart.length();
	for (int i = 0; i < len; ++i) {
		if (i > 0 && (len - i) % 3 == 0) {
			formattedInt += TQChar(0x00A0);
		}
		formattedInt += intPart[i];
	}

	return formattedInt + fracPart;
}

double Calculator::parseVal(const TQString &s)
{
	TQString copy = s;
	copy.remove(' ');
	copy.remove(TQChar(0x00A0));
	copy.replace(',', '.');
	return copy.toDouble();
}

void Calculator::setVal1Text(const TQString &text)
{
	if (m_lblVal1) m_lblVal1->setText(text);
	if (m_lblVal1_lcd) {
		m_lblVal1_lcd->clear();
		m_lblVal1_lcd->string(text.rightJustify(m_lblVal1_lcd->currentColumn(), ' ', true));
	}
}

void Calculator::setVal2Text(const TQString &text)
{
	if (m_lblVal2) m_lblVal2->setText(text);
	if (m_lblVal2_lcd) {
		m_lblVal2_lcd->clear();
		m_lblVal2_lcd->string(text.rightJustify(m_lblVal2_lcd->currentColumn(), ' ', true));
	}
}

TQString Calculator::converterTitle(CalcMode mode) const
{
	switch (mode) {
		case ModeVolume: return tr_str("Volume");
		case ModeLength: return tr_str("Length");
		case ModeMass: return tr_str("Weight and mass");
		case ModeTemp: return tr_str("Temperature");
		case ModeEnergy: return tr_str("Energy");
		case ModeArea: return tr_str("Area");
		case ModeSpeed: return tr_str("Speed");
		case ModeTime: return tr_str("Time");
		case ModePower: return tr_str("Power");
		case ModeData: return tr_str("Data");
		case ModePressure: return tr_str("Pressure");
		case ModeAngle: return tr_str("Angle");
		case ModeRoman: return tr_str("Roman numerals");
		default: return "";
	}
}

void Calculator::populateConverterUnits(CalcMode mode)
{
	if (!m_comboUnit1 || !m_comboUnit2) return;

	m_comboUnit1->blockSignals(true);
	m_comboUnit2->blockSignals(true);

	m_comboUnit1->clear();
	m_comboUnit2->clear();

	TQStringList units;
	switch (mode) {
		case ModeVolume:
			units << "Milliliters" << "Cubic centimeters" << "Liters" << "Cubic meters"
			      << "Cubic inches" << "Cubic feet" << "Cups (U.S.)" << "Pints (U.S.)"
			      << "Quarts (U.S.)" << "Gallons (U.S.)" << "Gallons (Imp.)" << "Bathtubs";
			break;
		case ModeLength:
			units << "Millimeters" << "Centimeters" << "Meters" << "Kilometers"
			      << "Inches" << "Feet" << "Yards" << "Miles" << "Nautical miles";
			break;
		case ModeMass:
			units << "Grams" << "Kilograms" << "Tonnes" << "Ounces"
			      << "Pounds" << "Stones" << "Tons (U.S. short)" << "Carats";
			break;
		case ModeTemp:
			units << "Celsius" << "Fahrenheit" << "Kelvin";
			break;
		case ModeEnergy:
			units << "Joules" << "Kilojoules" << "Calories" << "Kilocalories"
			      << "Watt-hours" << "Kilowatt-hours" << "Electronvolts"
			      << "BTUs" << "Foot-pounds";
			break;
		case ModeArea:
			units << "Square millimeters" << "Square centimeters" << "Square meters"
			      << "Square kilometers" << "Square inches" << "Square feet"
			      << "Square yards" << "Acres" << "Hectares";
			break;
		case ModeSpeed:
			units << "Meters per second" << "Kilometers per hour"
			      << "Miles per hour" << "Knots" << "Mach";
			break;
		case ModeTime:
			units << "Milliseconds" << "Seconds" << "Minutes" << "Hours"
			      << "Days" << "Weeks" << "Years";
			break;
		case ModePower:
			units << "Watts" << "Kilowatts" << "Megawatts" << "Horsepower"
			      << "Calories per second" << "BTUs per hour";
			break;
		case ModeData:
			units << "Bits" << "Bytes" << "Kilobytes" << "Megabytes"
			      << "Gigabytes" << "Terabytes" << "Petabytes";
			break;
		case ModePressure:
			units << "Pascals" << "Kilopascals" << "Bars" << "Atmospheres"
			      << "PSI (lb/in²)" << "mmHg";
			break;
		case ModeAngle:
			units << "Degrees" << "Radians" << "Gradians";
			break;
		case ModeRoman:
			// Handled separately below to populate single items
			break;
		default:
			break;
	}

	if (mode == ModeRoman) {
		m_comboUnit1->insertItem(tr_str("Arabic"));
		m_comboUnit2->insertItem(tr_str("Roman"));
	} else {
		for (TQStringList::ConstIterator it = units.begin(); it != units.end(); ++it) {
			m_comboUnit1->insertItem(tr_str((*it).latin1()));
			m_comboUnit2->insertItem(tr_str((*it).latin1()));
		}
	}

	int idx = mode - ModeVolume;
	if (idx >= 0 && idx < 13) {
		if (mode == ModeRoman) {
			m_comboUnit1->setCurrentItem(0);
			m_comboUnit2->setCurrentItem(0);
		} else {
			m_comboUnit1->setCurrentItem(m_convSelectedUnit1[idx]);
			m_comboUnit2->setCurrentItem(m_convSelectedUnit2[idx]);
		}
	}

	m_comboUnit1->blockSignals(false);
	m_comboUnit2->blockSignals(false);
}

double Calculator::getConversionFactor(CalcMode mode, int unitIndex) const
{
	switch (mode) {
		case ModeVolume:
			switch (unitIndex) {
				case 0: return 0.001;          // Milliliter(s)
				case 1: return 0.001;          // Cubic centimeter(s)
				case 2: return 1.0;            // Liter(s)
				case 3: return 1000.0;         // Cubic meter(s)
				case 4: return 0.016387064;    // Cubic inch(es)
				case 5: return 28.316846592;   // Cubic foot/feet
				case 6: return 0.2365882365;   // Cup(s) (U.S.)
				case 7: return 0.473176473;    // Pint(s) (U.S.)
				case 8: return 0.946352946;    // Quart(s) (U.S.)
				case 9: return 3.785411784;    // Gallon(s) (U.S.)
				case 10: return 4.54609;       // Gallon(s) (Imp.)
				case 11: return 378.5411784;   // Bathtub(s)
				default: return 1.0;
			}
		case ModeLength:
			switch (unitIndex) {
				case 0: return 0.001;          // Millimeters
				case 1: return 0.01;           // Centimeters
				case 2: return 1.0;            // Meters
				case 3: return 1000.0;         // Kilometers
				case 4: return 0.0254;         // Inches
				case 5: return 0.3048;         // Feet
				case 6: return 0.9144;         // Yards
				case 7: return 1609.344;       // Miles
				case 8: return 1852.0;         // Nautical miles
				default: return 1.0;
			}
		case ModeMass:
			switch (unitIndex) {
				case 0: return 0.001;          // Grams
				case 1: return 1.0;            // Kilograms
				case 2: return 1000.0;         // Tonnes
				case 3: return 0.028349523;    // Ounces
				case 4: return 0.45359237;     // Pounds
				case 5: return 6.35029318;     // Stones
				case 6: return 907.18474;      // Tons (short)
				case 7: return 0.0002;         // Carats
				default: return 1.0;
			}
		case ModeEnergy:
			switch (unitIndex) {
				case 0: return 1.0;            // Joules
				case 1: return 1000.0;         // Kilojoules
				case 2: return 4.184;          // Calories
				case 3: return 4184.0;         // Kilocalories
				case 4: return 3600.0;         // Watt-hours
				case 5: return 3600000.0;      // Kilowatt-hours
				case 6: return 1.602176634e-19;// Electronvolts
				case 7: return 1055.05585;     // BTUs
				case 8: return 1.3558179;      // Foot-pounds
				default: return 1.0;
			}
		case ModeArea:
			switch (unitIndex) {
				case 0: return 1e-6;           // Square mm
				case 1: return 1e-4;           // Square cm
				case 2: return 1.0;            // Square meters
				case 3: return 1000000.0;      // Square km
				case 4: return 0.00064516;     // Square inches
				case 5: return 0.09290304;     // Square feet
				case 6: return 0.83612736;     // Square yards
				case 7: return 4046.8564224;   // Acres
				case 8: return 10000.0;        // Hectares
				default: return 1.0;
			}
		case ModeSpeed:
			switch (unitIndex) {
				case 0: return 1.0;            // m/s
				case 1: return 1.0 / 3.6;      // km/h
				case 2: return 0.44704;        // mph
				case 3: return 0.514444;       // Knots
				case 4: return 343.0;          // Mach
				default: return 1.0;
			}
		case ModeTime:
			switch (unitIndex) {
				case 0: return 0.001;          // Milliseconds
				case 1: return 1.0;            // Seconds
				case 2: return 60.0;           // Minutes
				case 3: return 3600.0;         // Hours
				case 4: return 86400.0;        // Days
				case 5: return 604800.0;       // Weeks
				case 6: return 31536000.0;     // Years
				default: return 1.0;
			}
		case ModePower:
			switch (unitIndex) {
				case 0: return 1.0;            // Watts
				case 1: return 1000.0;         // Kilowatts
				case 2: return 1000000.0;      // Megawatts
				case 3: return 745.699872;     // Horsepower
				case 4: return 4.184;          // cal/s
				case 5: return 0.293071;       // BTU/h
				default: return 1.0;
			}
		case ModeData:
			switch (unitIndex) {
				case 0: return 0.125;          // Bits
				case 1: return 1.0;            // Bytes
				case 2: return 1024.0;         // KB
				case 3: return 1048576.0;      // MB
				case 4: return 1073741824.0;   // GB
				case 5: return 1099511627776.0;// TB
				case 6: return 1125899906842624.0; // PB
				default: return 1.0;
			}
		case ModePressure:
			switch (unitIndex) {
				case 0: return 1.0;            // Pascals
				case 1: return 1000.0;         // kPa
				case 2: return 100000.0;       // Bars
				case 3: return 101325.0;       // Atmospheres
				case 4: return 6894.75729;     // PSI
				case 5: return 133.322387;     // mmHg
				default: return 1.0;
			}
		case ModeAngle:
			switch (unitIndex) {
				case 0: return 0.017453292519943295; // Degrees
				case 1: return 1.0;                  // Radians
				case 2: return 0.015707963267948967; // Gradians
				default: return 1.0;
			}
		default:
			return 1.0;
	}
}

void Calculator::updateConversionMimeIcons()
{
	struct MimeIcon {
		const char *name;
		const unsigned char *data;
		unsigned int len;
	};
	MimeIcon mime_icons[] = {
		{ "banana", banana_png, banana_png_len },
		{ "bathtube", bathtube_png, bathtube_png_len },
		{ "battery", battery_png, battery_png_len },
		{ "cakeslice", cakeslice_png, cakeslice_png_len },
		{ "castle", castle_png, castle_png_len },
		{ "elephant", elephant_png, elephant_png_len },
		{ "football", football_png, football_png_len },
		{ "hand", hand_png, hand_png_len },
		{ "horse", horse_png, horse_png_len },
		{ "jet", jet_png, jet_png_len },
		{ "papersheet", papersheet_png, papersheet_png_len },
		{ "snowflake", snowflake_png, snowflake_png_len },
		{ "swimmingpool", swimmingpool_png, swimmingpool_png_len },
		{ "teacup", teacup_png, teacup_png_len },
		{ "turtle", turtle_png, turtle_png_len },
		{ "whale", whale_png, whale_png_len }
	};
	for (size_t i = 0; i < sizeof(mime_icons)/sizeof(mime_icons[0]); ++i) {
		TQImage img = IconUtils::load(mime_icons[i].data, mime_icons[i].len).convertToImage();
		TQMimeSourceFactory::defaultFactory()->setImage(mime_icons[i].name, img);
	}
}

TQString Calculator::arabicToRoman(int val)
{
	if (val == 0) return "N";
	if (val < 1 || val > 3999) return tr_str("Out of range");

	struct RomanMapping {
		int value;
		const char *symbol;
	};
	static const RomanMapping mappings[] = {
		{1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"},
		{100, "C"}, {90, "XC"}, {50, "L"}, {40, "XL"},
		{10, "X"}, {9, "IX"}, {5, "V"}, {4, "IV"},
		{1, "I"}
	};

	TQString result = "";
	int remaining = val;
	for (int i = 0; i < 13; ++i) {
		while (remaining >= mappings[i].value) {
			result += mappings[i].symbol;
			remaining -= mappings[i].value;
		}
	}
	return result;
}

int Calculator::romanToArabic(const TQString &roman)
{
	TQString r = roman.upper().stripWhiteSpace();
	if (r.isEmpty()) return 0;
	if (r == "N") return 0;

	// Basic character check
	for (unsigned int i = 0; i < r.length(); ++i) {
		char c = r[i].latin1();
		if (c != 'I' && c != 'V' && c != 'X' && c != 'L' && c != 'C' && c != 'D' && c != 'M') {
			return -1;
		}
	}

	int total = 0;
	int lastValue = 0;
	// Process from right to left
	for (int i = static_cast<int>(r.length()) - 1; i >= 0; --i) {
		char c = r[i].latin1();
		int value = 0;
		switch (c) {
			case 'I': value = 1; break;
			case 'V': value = 5; break;
			case 'X': value = 10; break;
			case 'L': value = 50; break;
			case 'C': value = 100; break;
			case 'D': value = 500; break;
			case 'M': value = 1000; break;
			default: return -1;
		}
		if (value < lastValue) {
			total -= value;
		} else {
			total += value;
		}
		lastValue = value;
	}

	// Validate using round-trip
	TQString roundTrip = arabicToRoman(total);
	if (roundTrip != r) {
		return -1;
	}
	return total;
}

