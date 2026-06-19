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

#ifndef CALC_H
#define CALC_H

#include <vector>

class TQPushButton;
class TQRadioButton;
class TQButtonGroup;
class TQHButtonGroup;
class TQWidget;
class TQComboBox;
class TQSpinBox;
class DispLogic;
class Constants;
class TQtDatePeriodPicker;
class TQtLcdWidget;
class CalcButton;
#include <tqmainwindow.h>
#include <tqdatetime.h>
#include <tqvaluelist.h>
#include <tqscrollview.h>
#include <tqlabel.h>
#include <tqpushbutton.h>
#include <tqdict.h>
#include <tdeaction.h>
#include <tqhbuttongroup.h>
#include "calc_core.h"
#include "calc_button.h"
#include "calc_const_button.h"

class Calculator;

class ProgMemoryRowWidget : public TQWidget {
	TQ_OBJECT
public:
	ProgMemoryRowWidget(const KNumber &val, int index, Calculator *calc, TQWidget *parent = 0);
	void updateRowSizes(double scale);
	void updateColors();
protected:
	virtual void enterEvent(TQEvent *e);
	virtual void leaveEvent(TQEvent *e);
	virtual void paintEvent(TQPaintEvent *e);
	virtual void mousePressEvent(TQMouseEvent *e);
private slots:
	void slotMCClicked();
	void slotMPlusClicked();
	void slotMMinusClicked();
private:
	KNumber m_val;
	int m_index;
	Calculator *m_calc;
	TQLabel *m_lblValue;
	TQWidget *m_btnContainer;
	CalcButton *m_btnMC;
	CalcButton *m_btnMPlus;
	CalcButton *m_btnMMinus;
};

/*
  Kcalc basically consist of a class for the GUI (here), a class for
  the display (dlabel.h), and one for the mathematics core
  (calc_core.h).

  When for example '+' is pressed, one sends the contents of the
  Display and the '+' to the core via "core.Plus(DISPLAY_AMOUNT)".
  This only updates the core. To bring the changes to the display,
  use afterwards "UpdateDisplay(true)".

  "UpdateDisplay(true)" means that the amount to be displayed should
  be taken from the core (get the result of some operation that was
  performed), "UpdateDisplay(false)" has already the information, what
  to be display (e.g. user is typing in a number).  Note that in the
  last case the core does not know the number typed in until some
  operation button is pressed, e.g. "core.Plus(display_number)".
 */

class CalcCentralWidget : public TQWidget {
	TQ_OBJECT
public:
	CalcCentralWidget(Calculator *calc, TQWidget *parent = 0);
protected:
	virtual void paintEvent(TQPaintEvent *e);
private:
	Calculator *m_calc;
};

class HistoryWindow;

struct CalcHistoryItem {
    TQString expression;
    TQString result_str;
    KNumber result_num;
};

class Calculator : public TQMainWindow
{
    TQ_OBJECT
  

public:
	Calculator(TQWidget *parent = 0, const char *name = 0);
	~Calculator();

	virtual TQSize minimumSizeHint() const {
		return TQSize(350, 400);
	}

	virtual TQSize sizeHint() const {
		return TQSize(470, 500);
	}

	void recallMemoryEntry(const KNumber &val);

	bool hasWindowBorder() const { return m_windowBorder; }
	TQColor windowBorderColor() const { return m_windowBorderColor; }
	TQColor panelBackgroundColor() const { return m_colorPanelBg; }
	TQColor panelForegroundColor() const { return m_colorPanelFg; }
	TQColor menuBackgroundColor() const { return m_colorMenuBg; }

signals:
	void switchInverse(bool);
	void switchMode(ButtonModeFlags,bool);
	void switchShowAccels(bool);

private:
	virtual bool eventFilter( TQObject *o, TQEvent *e );
	void updateGeometry();
	void setupMainActions(void);
	void setupStatusbar(void);
	TQWidget *setupNumericKeys(TQWidget *parent);
	TQWidget *setupStandardKeys(TQWidget *parent);
	void setupHeaderBar(TQWidget *parent);
	void setupMenuBar();
	void setupLogicKeys(TQWidget *parent);
	void setupScientificKeys(TQWidget *parent);
	void setupStatisticKeys(TQWidget *parent);
	void setupConstantsKeys(TQWidget *parent);
	void keyPressEvent(TQKeyEvent *e);
	void keyReleaseEvent(TQKeyEvent *e);
	virtual void resizeEvent(TQResizeEvent *e);
	virtual void showEvent(TQShowEvent *e);
	void set_precision();
	void set_style();
	void resetBase(void) { (BaseChooseGroup->find(1))->animateClick();};

	void UpdateDisplay(bool get_amount_from_core = false,
			   bool store_result_in_history = false);
	void updateModeVisibility();

protected slots:
    void changeButtonNames();
    void retranslateUi();
    void updateSettings();
    void set_colors(bool clear_cache = false);
    void EnterEqual();
    void showSettings();
    void slotStatshow(bool toggled);
    void slotScientificshow(bool toggled);
    void slotLogicshow(bool toggled);
    void slotConstantsShow(bool toggled);   
    void slotShowAll(void);
    void slotHideAll(void);
    void slotAngleSelected(int number);
    void slotBaseSelected(int number);
    void slotNumberclicked(int number_clicked);
    void slotEEclicked(void);
    void slotInvtoggled(bool myboolean);
    void slotStdMPlusclicked(void);
    void slotStdMMinusclicked(void);
    void slotHamburgerClicked(void);
    void slotScientificFEtoggled(bool toggled);
    void slotHistoryClicked();
    void slotHistoryItemSelected(const CalcHistoryItem &item);
    void slotClearHistory();

    void slotWordSizeClicked(void);
    void slotKeypadViewClicked(void);
    void slotBitboardViewClicked(void);
    void slotBitboardClicked(int id);
    void updateBitboard(void);
    void updateBitboardLabelSizes(void);
    void slotSciAngleClicked(void);
    void slotPinClicked(void);
    void slotMinimizeClicked();
    void slotMaximizeClicked();
    void slotCloseClicked();
    void slotUpdateDisplayMenu();
    void slotDisplayMenuActivated(int id);
    void slotMenuStandard();
    void slotMenuScientific();
    void slotMenuProgrammer();
    void slotMenuDateCalc();
    void slotMenuVolume();
    void slotMenuConfigure();
    void slotMenuAbout();
    void slotToggleProgMemoryFrame(void);
    void slotProgMemStoreclicked(void);
    void slotProgMCclicked(void);

    void slotDateModeChanged(int index);
    void slotDateFromChanged(const TQDate &date);
    void slotDateToChanged(const TQDate &date);
    void slotDateInputChanged(void);
    void updateDateDiffResult(void);
    void slotConvUnit1Changed(int index);
    void slotConvUnit2Changed(int index);
    void slotUpdateEquivGeometry();
    void slotConvDigitClicked();
    void slotConvCommaClicked();
    void slotConvCEClicked();
    void slotConvBSClicked();
    void slotBackSpaceclicked(void);
    void slotMemRecallclicked(void);
    void slotMemStoreclicked(void);
    void slotSinclicked(void);
    void slotPlusMinusclicked(void);
    void slotMemPlusMinusclicked(void);
    void slotCosclicked(void);
    void slotReciclicked(void);
    void slotTanclicked(void);
    void slotFactorialclicked(void);
    void slotLogclicked(void);
    void slot10xclicked(void);
    void slotPiclicked(void);
    void slotSquareclicked(void);
    void slotLnclicked(void);
    void slotPowerclicked(void);
    void slotMCclicked(void);
    void slotClearclicked(void);
    void slotACclicked(void);
    void slotParenOpenclicked(void);
    void slotParenCloseclicked(void);
    void slotANDclicked(void);
    void slotXclicked(void);
    void slotDivisionclicked(void);
    void slotORclicked(void);
    void slotXORclicked(void);
    void slotPlusclicked(void);
    void slotMinusclicked(void);
    void slotLeftShiftclicked(void);
    void slotRightShiftclicked(void);
    void slotPeriodclicked(void);
    void slotEqualclicked(void);
    void slotPercentclicked(void);
    void slotRootclicked(void);
    void slotNegateclicked(void);
    void slotModclicked(void);
    void slotStatNumclicked(void);
    void slotStatMeanclicked(void);
    void slotStatStdDevclicked(void);
    void slotStatMedianclicked(void);
    void slotStatDataInputclicked(void);
    void slotStatClearDataclicked(void);
    void slotHyptoggled(bool flag);
    void slotConstclicked(int);

    void slotConstantToDisplay(int constant);
    void slotChooseScientificConst0(int option);
    void slotChooseScientificConst1(int option);
    void slotChooseScientificConst2(int option);
    void slotChooseScientificConst3(int option);
    void slotChooseScientificConst4(int option);
    void slotChooseScientificConst5(int option);

private:
	bool inverse;
	bool hyp_mode;
	KNumber memory_num;
	KNumber setvalue;

	// angle modes for trigonometric values
	enum {
	  DegMode,
	  RadMode,
	  GradMode
	} _angle_mode;

	int _word_size; // 0=QWORD, 1=DWORD, 2=WORD, 3=BYTE

    void updateDateCalcPageSizes();

private:
    TQWidget *mSmallPage;
    TQWidget *mLargePage;
    TQWidget *mNumericPage;
    TQWidget *mHeaderWidget;
    CalcButton *pbHamburger;
    CalcButton *pbConstantsMenu;
    CalcButton *pbHistory;
    CalcButton *pbPin;
    CalcButton *pbMinimize;
    CalcButton *pbMaximize;
    CalcButton *pbClose;
    TQLabel *lblModeTitle;
    TQPopupMenu *displayMenu;
    TQPopupMenu *settingsMenu;

    TQWidget *mStandardPage;
    CalcButton *pbStdMC;
    CalcButton *pbStdMR;
    CalcButton *pbStdMPlus;
    CalcButton *pbStdMMinus;
    CalcButton *pbStdMS;

    TQWidget *mStandardPageGrid;
    TQWidget *mScientificPage;
    TQWidget *mScientificPageGrid;
    TQButtonGroup *SciNumButtonGroup;

    TQWidget *mProgrammerPage;
    TQWidget *mProgrammerPageGrid;
    TQWidget *mBitboardPage;
    TQButtonGroup *ProgNumButtonGroup;
    TQButtonGroup *BitboardButtonGroup;
    TQLabel *mBitboardLabels[16];
    CalcButton *pbProgWordSize;
    TQPtrList<TQWidget> mProgKeypadWidgets;  // non-toolbar widgets for keypad/bitboard toggle

    TQWidget *mDateCalcPage;
    TQComboBox *mDateModeCombo;
    TQtDatePeriodPicker *mDateFromPicker;
    TQtDatePeriodPicker *mDateToPicker;
    TQLabel *mDateDiffLabel;
    TQLabel *mDateDiffResult;
    TQLabel *mDateDiffResultDays;
    TQLabel *lblFrom;
    TQLabel *lblTo;
    TQRadioButton *mAddRadio;
    TQRadioButton *mSubRadio;
    TQLabel *lblYears;
    TQLabel *lblMonths;
    TQLabel *lblDays;
    TQSpinBox *mYearsCombo;
    TQSpinBox *mMonthsCombo;
    TQSpinBox *mDaysCombo;
    TQHBoxLayout *mRadioLayout;
    TQHBoxLayout *mOffsetLayout;
    TQVBoxLayout *colYears;
    TQVBoxLayout *colMonths;
    TQVBoxLayout *colDays;

    CalcButton *pbSciAngle;
    CalcButton *pbSciHyp;
    CalcButton *pbSciFE;

    CalcButton *pbSciInv;
    CalcButton *pbSciSin;
    CalcButton *pbSciCos;
    CalcButton *pbSciTan;
    CalcButton *pbSciParenL;
    CalcButton *pbSciParenR;
    CalcButton *pbSciEq;
    CalcButton *pbSciC;
    CalcButton *pbSciCE;
    CalcButton *pbSciBS;
    CalcButton *pbSciDiv;
    CalcButton *pbSciMul;
    CalcButton *pbSciSub;
    CalcButton *pbSciAdd;
    CalcButton *pbSciDot;
    CalcButton *pbSciPM;

    CalcButton *pbProgEq;
    CalcButton *pbProgC;
    CalcButton *pbProgCE;
    CalcButton *pbProgBS;
    CalcButton *pbProgDiv;
    CalcButton *pbProgMul;
    CalcButton *pbProgSub;
    CalcButton *pbProgAdd;
    CalcButton *pbProgPM;
    CalcButton *pbProgParenL;
    CalcButton *pbProgParenR;

    CalcButton *pbStdPercent;
    CalcButton *pbStdRoot;
    CalcButton *pbStdSquare;
    CalcButton *pbStdReci;
    CalcButton *pbStdCE;
    CalcButton *pbStdC;
    CalcButton *pbStdBackSpace;
    CalcButton *pbStdDivision;
    CalcButton *pbStdX;
    CalcButton *pbStdMinus;
    CalcButton *pbStdPlus;
    CalcButton *pbStdEqual;
    CalcButton *pbStdPlusMinus;
    CalcButton *pbStdPeriod;
    TQButtonGroup *StdNumButtonGroup;

    DispLogic*	calc_display; // for historic reasons in "dlabel.h"
    TQRadioButton*	pbBaseChoose[4];
    CalcButton*	pbAngleChoose;
    TQDict<CalcButton>	pbStat;
    TQDict<CalcButton>	pbScientific;
    TQDict<CalcButton>	pbLogic;
    CalcConstButton*	pbConstant[10];
    CalcButton* 	pbAC;
    CalcButton* 	pbAND;
    CalcButton* 	pbClear;
    CalcButton* 	pbDivision;
    CalcButton* 	pbEE;
    CalcButton* 	pbEqual;
    CalcButton* 	pbFactorial;
    CalcButton* 	pbInv;
    CalcButton* 	pbMC;
    CalcButton* 	pbMinus;
    CalcButton* 	pbMod;
    CalcButton* 	pbMemPlusMinus;
    CalcButton* 	pbMemRecall;
    CalcButton*	pbMemStore;
    CalcButton* 	pbOR;
    CalcButton* 	pbParenClose;
    CalcButton* 	pbParenOpen;
    CalcButton* 	pbPercent;
    CalcButton* 	pbPeriod;
    CalcButton* 	pbPlus;
    CalcButton* 	pbPlusMinus;
    CalcButton* 	pbPower;
    CalcButton* 	pbReci;
    KSquareButton* 	pbRoot;
    CalcButton* 	pbSquare;
    CalcButton* 	pbX;
    CalcButton* 	pbXOR;

    Constants * tmp_const; // this is the dialog for configuring const
			   // buttons would like to remove this, but
			   // don't know how
	
    TQHButtonGroup *BaseChooseGroup;
    TQButtonGroup  *NumButtonGroup;
    TQButtonGroup  *ConstButtonGroup;

    TDEToggleAction *actionStatshow;
    TDEToggleAction *actionScientificshow;
    TDEToggleAction *actionLogicshow;
    TDEToggleAction *actionConstantsShow;

    TQPtrList<CalcButton> mFunctionButtonList;
    TQPtrList<CalcButton> mStatButtonList;
    TQPtrList<CalcButton> mMemButtonList;
    TQPtrList<CalcButton> mOperationButtonList;

    int				mInternalSpacing;

    bool            is_pinned;
    bool            m_noDeco;
    bool            m_windowBorder;
    TQColor         m_windowBorderColor;

    // Expression display state (history line)
    TQString         _expr_text;        // The expression string shown on the history line
    TQString         _expr_unary;       // Unary wrapper for current operand (e.g. "sqrt(9)")
    bool            _expr_just_equaled; // True right after '=' was pressed
    bool            _expr_op_pending;   // True when expression ends with an operator

    enum CalcMode {
        ModeStandard,
        ModeScientific,
        ModeProgrammer,
        ModeStatistics,
        ModeDateCalc,
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
    CalcMode _calc_mode;

    // Helper to format a KNumber for the expression display
    TQString exprFormatNumber(const KNumber &n) const;
    // Helper for binary operator expression update (Standard mode, sequential)
    void exprBinaryOp(const TQString &op_symbol);
    void exprUnaryOp(const TQString &func_name);

    TQWidget* setupScientificKeys_win10(TQWidget *parent);
    TQWidget* setupProgrammerKeys_win10(TQWidget *parent);
    void setupDateCalcPage(TQWidget *parent);
    void setupConverterPage(TQWidget *parent);
    void updateConverterSizes();
    void handleConvDigit(int digit);
    void handleConvComma();
    void handleConvCE();
    void handleConvBS();
    double getVolumeFactor(int index);
    TQString converterTitle(CalcMode mode) const;
    void populateConverterUnits(CalcMode mode);
    double getConversionFactor(CalcMode mode, int unitIndex) const;
    void recalculateConversion();
    void updateConversionMimeIcons();
    TQString formatDouble(double val);
    double parseVal(const TQString &s);
    void setActiveSlot(int slot);
    void setVal1Text(const TQString &text);
    void setVal2Text(const TQString &text);
    TQString arabicToRoman(int val);
    int romanToArabic(const TQString &roman);

    // Converter state
    int m_convActiveSlot;
    TQString m_convInput1;
    TQString m_convInput2;
    int m_convUnit1;
    int m_convUnit2;
    int m_currentLoadedModeIdx;

    // State arrays for 13 converter modes
    int m_convSelectedUnit1[13];
    int m_convSelectedUnit2[13];
    TQString m_convSelectedInput1[13];
    TQString m_convSelectedInput2[13];
    int m_convSelectedActiveSlot[13];

    // Converter UI elements
    TQWidget *mConverterPage;
    TQWidget *m_converterTopPanel;
    TQLabel *m_lblVal1;
    TQLabel *m_lblVal2;
    TQtLcdWidget *m_lblVal1_lcd;
    TQtLcdWidget *m_lblVal2_lcd;
    int m_displayType;
    TQFont m_displayFont;
    TQColor m_colorDisplayBg;
    TQColor m_colorDisplayFg;
    TQColor m_colorPanelBg;
    TQColor m_colorPanelFg;
    TQColor m_colorMenuBg;
    bool m_invertIcons;
    TQComboBox *m_comboUnit1;
    TQComboBox *m_comboUnit2;
    TQLabel *m_lblEquiv;
    TQLabel *m_lblEquivTitle;
    CalcButton *m_btnConvDigits[10];
    CalcButton *m_btnConvComma;
    CalcButton *m_btnConvCE;
    CalcButton *m_btnConvBS;
	
public:
    void deleteProgMemoryEntry(int index);
    void addProgMemoryEntry(int index, const KNumber &val);
    void subtractProgMemoryEntry(int index, const KNumber &val);
    TQString formatMemoryValue(const KNumber &val);
    KNumber currentDisplayValue() const;

private:
    TQWidget* createMemoryFrame(TQWidget *parent, TQScrollView *&outScroll);
    void rebuildProgMemoryFrame();
    void updateProgMemoryViews();
    void updateProgMemorySizes();
    void setProgrammerGridVisible(bool visible);

    CalcButton *btnProgM;
    CalcButton *mProgMemoryDeleteButton;
    TQWidget *mProgrammerMemoryFrame;
    TQScrollView *mProgMemoryFrameScroll;
    TQWidget *mProgMemoryContainer;
    TQWidget *mProgMemoryBottomBar;
    bool m_programmerMemoryFrameVisible;
    bool m_programmer_bitboard_view;
    TQValueList<KNumber> m_programmerMemoryStack;

    HistoryWindow *m_historyWindow;
    std::vector<CalcHistoryItem> m_history[4];

    bool handleKeyPress(TQKeyEvent *e);

    CalcEngine core;
    bool m_isDragging;
    TQPoint m_dragOffset;
};

#endif  // CALC_H
