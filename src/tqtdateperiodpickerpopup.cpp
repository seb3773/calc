#include "tqtdateperiodpickerpopup.h"

#include "tqtdateperiodpickercalendarpanel.h"
#include "tqtdateperiodpickerpopupfooter.h"

#include <ntqapplication.h>
#include <ntqdatetimeedit.h>
#include <ntqdesktopwidget.h>
#include <ntqframe.h>
#include <ntqlayout.h>
#include <ntqpainter.h>
#include <ntqobjectlist.h>

TQtDatePeriodPickerPopup::TQtDatePeriodPickerPopup(TQWidget* parent)
    : TQWidget(parent, 0, (WType_Popup | WStyle_Customize | WStyle_NoBorder)),
      m_syncingScroll(0),
      m_timeEditable(0),
      m_pickerType((int)TQtDPPDayType),
      m_allowedTypes(TQtDPPDayType | TQtDPPPeriodType),
      m_begin(TQDate::currentDate()),
      m_end(TQDate::currentDate()),
      m_timeBegin(TQTime(0, 0, 0)),
      m_timeEnd(TQTime(23, 59, 59)),
      m_cal1(0),
      m_cal2(0),
      m_te1(0),
      m_te2(0),
      m_footer(0)
{
    setBackgroundMode(NoBackground);
    setFocusPolicy(StrongFocus);

    TQFont f = font();
    f.setPointSize(9);
    setFont(f);

    m_cal1 = new TQtDatePeriodPickerCalendarPanel(this);
    m_cal2 = new TQtDatePeriodPickerCalendarPanel(this);

    m_te1 = new TQTimeEdit(this);
    m_te2 = new TQTimeEdit(this);

    m_footer = new TQtDatePeriodPickerPopupFooter(this);

    TQGridLayout* grid = new TQGridLayout(2, 2);
    grid->setMargin(0);
    grid->setSpacing(6);
    grid->addWidget(m_cal1, 0, 0);
    grid->addWidget(m_cal2, 0, 1);
    grid->addWidget(m_te1, 1, 0);
    grid->addWidget(m_te2, 1, 1);

    TQVBoxLayout* lay = new TQVBoxLayout(this);
    lay->setMargin(12);
    lay->setSpacing(6);
    lay->addLayout(grid, 1);
    m_footer->hide();

    connect(m_cal1, SIGNAL(dateSelected(const TQDate&)), this, SLOT(onCalendar1DateSelected_(const TQDate&)));
    connect(m_cal2, SIGNAL(dateSelected(const TQDate&)), this, SLOT(onCalendar2DateSelected_(const TQDate&)));

    connect(m_cal1, SIGNAL(scrolledTo(const TQDate&)), this, SLOT(onCalendar1Scrolled_(const TQDate&)));
    connect(m_cal2, SIGNAL(scrolledTo(const TQDate&)), this, SLOT(onCalendar2Scrolled_(const TQDate&)));

    connect(m_te1, SIGNAL(valueChanged(const TQTime&)), this, SLOT(onTimeEdit1TimeChanged_(const TQTime&)));
    connect(m_te2, SIGNAL(valueChanged(const TQTime&)), this, SLOT(onTimeEdit2TimeChanged_(const TQTime&)));

    connect(m_footer, SIGNAL(pickerTypeChanged(int)), this, SLOT(setDatePickerType(int)));
    connect(m_footer, SIGNAL(okClicked()), this, SLOT(accept()));

    m_te1->setTime(m_timeBegin);
    m_te2->setTime(m_timeEnd);

    setDate(m_begin);
    updateTypeUi_();
    updateTimeUi_();

    setCaption("Date Picker");
}

TQtDatePeriodPickerPopup::~TQtDatePeriodPickerPopup() {}

int TQtDatePeriodPickerPopup::isTimeEditable() const { return m_timeEditable; }

unsigned int TQtDatePeriodPickerPopup::timeDisplay() const
{
    if (!m_te1) return 0u;
    return m_te1->display();
}

int TQtDatePeriodPickerPopup::datePickerType() const { return m_pickerType; }
TQtDatePeriodPickerTypes TQtDatePeriodPickerPopup::allowedPickerTypes() const { return m_allowedTypes; }

TQDate TQtDatePeriodPickerPopup::date() const { return m_begin; }
TQDate TQtDatePeriodPickerPopup::periodBegin() const { return m_begin; }
TQDate TQtDatePeriodPickerPopup::periodEnd() const { return m_end; }

TQTime TQtDatePeriodPickerPopup::timeBegin() const { return m_timeBegin; }
TQTime TQtDatePeriodPickerPopup::timeEnd() const { return m_timeEnd; }

void TQtDatePeriodPickerPopup::setMinimumDate(const TQDate& date)
{
    if (m_cal1) m_cal1->setMinimumDate(date);
    if (m_cal2) m_cal2->setMinimumDate(date);
}

void TQtDatePeriodPickerPopup::setMaximumDate(const TQDate& date)
{
    if (m_cal1) m_cal1->setMaximumDate(date);
    if (m_cal2) m_cal2->setMaximumDate(date);
}

void TQtDatePeriodPickerPopup::setDate(const TQDate& date)
{
    setDateInternal_(date);
}

void TQtDatePeriodPickerPopup::setDatePeriod(const TQDate& begin, const TQDate& end)
{
    setDatePeriodInternal_(begin, end);
}

void TQtDatePeriodPickerPopup::setTimePeriod(const TQTime& begin, const TQTime& end)
{
    setTimePeriodInternal_(begin, end);
}

void TQtDatePeriodPickerPopup::setDatePickerType(int pickerType)
{
    const int old = m_pickerType;

    m_pickerType = pickerType;
    if (m_footer) m_footer->setPickerType(pickerType);
    updateTypeUi_();

    if (m_pickerType != old) emit datePickerTypeChanged(m_pickerType);
}

void TQtDatePeriodPickerPopup::setAllowedPickerTypes(TQtDatePeriodPickerTypes pickerTypes)
{
    const int old = m_pickerType;

    m_allowedTypes = pickerTypes;
    if (m_footer) m_footer->setAllowedPickerTypes(pickerTypes);
    updateTypeUi_();

    if (m_pickerType != old) emit datePickerTypeChanged(m_pickerType);
}

void TQtDatePeriodPickerPopup::setTimeEditable(int on)
{
    m_timeEditable = on ? 1 : 0;
    updateTimeUi_();
}

void TQtDatePeriodPickerPopup::setTimeDisplay(unsigned int disp)
{
    if (m_te1) m_te1->setDisplay(disp);
    if (m_te2) m_te2->setDisplay(disp);
}

void TQtDatePeriodPickerPopup::reset()
{
    TQColor bg, fg;
    dppGetPanelColors(bg, fg);

    TQPalette pal = palette();
    pal.setColor(TQPalette::Active, TQColorGroup::Background, bg);
    pal.setColor(TQPalette::Inactive, TQColorGroup::Background, bg);
    pal.setColor(TQPalette::Active, TQColorGroup::Foreground, fg);
    pal.setColor(TQPalette::Inactive, TQColorGroup::Foreground, fg);
    pal.setColor(TQPalette::Active, TQColorGroup::Text, fg);
    pal.setColor(TQPalette::Inactive, TQColorGroup::Text, fg);
    pal.setColor(TQPalette::Active, TQColorGroup::ButtonText, fg);
    pal.setColor(TQPalette::Inactive, TQColorGroup::ButtonText, fg);
    pal.setColor(TQPalette::Active, TQColorGroup::Base, bg);
    pal.setColor(TQPalette::Inactive, TQColorGroup::Base, bg);
    pal.setColor(TQPalette::Active, TQColorGroup::Button, bg);
    pal.setColor(TQPalette::Inactive, TQColorGroup::Button, bg);
    setPalette(pal);

    TQObjectList *childList = queryList("TQWidget");
    if (childList) {
        TQObjectListIt it(*childList);
        TQObject *obj;
        while ((obj = it.current()) != 0) {
            ++it;
            TQWidget *w = dynamic_cast<TQWidget*>(obj);
            if (w) {
                w->setPalette(pal);
                w->setBackgroundColor(bg);
            }
        }
        delete childList;
    }

    if (m_cal1) m_cal1->reset();
    if (m_cal2) m_cal2->reset();
}

void TQtDatePeriodPickerPopup::accept()
{
    close();
    emit accepted();
}

void TQtDatePeriodPickerPopup::setDateInternal_(const TQDate& date)
{
    m_begin = date;
    m_end = date;

    if (m_cal1) m_cal1->setDate(date);
    if (m_cal2) m_cal2->setDate(date.addMonths(1));

    updateSecondCalendarMonth_();
}

void TQtDatePeriodPickerPopup::setDatePeriodInternal_(const TQDate& begin, const TQDate& end)
{
    m_begin = begin;
    m_end = end;

    if (m_cal1) m_cal1->setPeriod(begin, end, 1);
    if (m_cal2) m_cal2->setPeriod(begin, end, 0);

    updateSecondCalendarMonth_();
}

void TQtDatePeriodPickerPopup::setTimePeriodInternal_(const TQTime& begin, const TQTime& end)
{
    m_timeBegin = begin;
    m_timeEnd = end;

    if (m_te1) m_te1->setTime(begin);
    if (m_te2) m_te2->setTime(end);
}

void TQtDatePeriodPickerPopup::updateTypeUi_()
{
    const int allowDay = (m_allowedTypes & TQtDPPDayType) ? 1 : 0;
    const int allowPeriod = (m_allowedTypes & TQtDPPPeriodType) ? 1 : 0;

    int type = m_pickerType;
    if (type == TQtDPPDayType && !allowDay) type = TQtDPPPeriodType;
    if (type == TQtDPPPeriodType && !allowPeriod) type = TQtDPPDayType;

    m_pickerType = type;

    if (m_footer) m_footer->setPickerType(m_pickerType);

    if (m_cal2) m_cal2->setShown((m_pickerType == TQtDPPPeriodType) ? true : false);

    if (m_pickerType == TQtDPPDayType) {
        if (m_cal1) m_cal1->setFrameVisible(0);
        if (m_cal2) m_cal2->setFrameVisible(0);
    } else {
        if (m_cal1) m_cal1->setFrameVisible(1);
        if (m_cal2) m_cal2->setFrameVisible(1);
    }

    updateTimeUi_();
    updateSecondCalendarMonth_();
}

void TQtDatePeriodPickerPopup::updateTimeUi_()
{
    const int show = (m_timeEditable && m_pickerType == TQtDPPPeriodType) ? 1 : 0;

    if (m_te1) m_te1->setShown(show ? true : false);
    if (m_te2) m_te2->setShown(show ? true : false);
}

void TQtDatePeriodPickerPopup::updateSecondCalendarMonth_()
{
    if (m_pickerType != TQtDPPPeriodType) {
        if (m_cal2) m_cal2->setDate((m_begin.isValid() ? m_begin : TQDate::currentDate()).addMonths(1));
        return;
    }

    if (!m_begin.isValid() || !m_end.isValid()) return;

    const int y0 = m_begin.year();
    const int m0 = m_begin.month();
    const int y1 = m_end.year();
    const int m1 = m_end.month();

    const int ym0 = y0 * 12 + (m0 - 1);
    const int ym1 = y1 * 12 + (m1 - 1);

    const int diff = ym1 - ym0;

    TQDate d2 = m_end;
    if (diff <= 0) d2 = m_begin.addMonths(1);
    else if (diff == 1) d2 = m_end;
    else d2 = m_begin.addMonths(1);

    if (m_cal2) m_cal2->setPeriod(m_begin, m_end, 0);
    if (m_cal2) m_cal2->setDate(d2);
}

void TQtDatePeriodPickerPopup::onCalendar1DateSelected_(const TQDate& date)
{
    if (m_pickerType == TQtDPPDayType) {
        setDateInternal_(date);
        emit dateSelected(date);
        accept();
        return;
    }

    TQDate b = date;
    TQDate e = m_end;

    if (!e.isValid()) e = date;

    if (b > e) {
        const TQDate t = b;
        b = e;
        e = t;
    }

    setDatePeriodInternal_(b, e);
    emit datePeriodSelected(b, e);
}

void TQtDatePeriodPickerPopup::onCalendar2DateSelected_(const TQDate& date)
{
    if (m_pickerType == TQtDPPDayType) {
        setDateInternal_(date);
        emit dateSelected(date);
        accept();
        return;
    }

    TQDate b = m_begin;
    TQDate e = date;

    if (!b.isValid()) b = date;

    if (b > e) {
        const TQDate t = b;
        b = e;
        e = t;
    }

    setDatePeriodInternal_(b, e);
    emit datePeriodSelected(b, e);
}

void TQtDatePeriodPickerPopup::onCalendar1Scrolled_(const TQDate& date)
{
    if (m_pickerType == TQtDPPPeriodType) {
        if (m_syncingScroll) return;
        if (!m_cal2) return;

        m_syncingScroll = 1;
        const TQDate d2 = date.addMonths(1);
        m_cal2->setCurrentPage(d2.year(), d2.month());
        m_syncingScroll = 0;
    }
}

void TQtDatePeriodPickerPopup::onCalendar2Scrolled_(const TQDate& date)
{
    if (m_pickerType == TQtDPPPeriodType) {
        if (m_syncingScroll) return;
        if (!m_cal1) return;

        m_syncingScroll = 1;
        const TQDate d1 = date.addMonths(-1);
        m_cal1->setCurrentPage(d1.year(), d1.month());
        m_syncingScroll = 0;
    }
}

void TQtDatePeriodPickerPopup::onTimeEdit1TimeChanged_(const TQTime& time)
{
    m_timeBegin = time;
    emit timePeriodSelected(m_timeBegin, m_timeEnd);
}

void TQtDatePeriodPickerPopup::onTimeEdit2TimeChanged_(const TQTime& time)
{
    m_timeEnd = time;
    emit timePeriodSelected(m_timeBegin, m_timeEnd);
}

void TQtDatePeriodPickerPopup::paintEvent(TQPaintEvent*)
{
    TQColor bg, fg;
    dppGetPanelColors(bg, fg);
    TQPainter p(this);
    p.fillRect(rect(), bg);
    p.setPen(fg);
    p.drawRect(0, 0, width() - 1, height() - 1);
}

void TQtDatePeriodPickerPopup::resizeEvent(TQResizeEvent* ev)
{
    TQWidget::resizeEvent(ev);
    
    int h = height();
    if (h < 50) return;
    
    int baseSize = h * 0.05; // 5% of popup height
    if (baseSize < 10) baseSize = 10;
    
    TQFont f = font();
    f.setPixelSize(baseSize);
    setFont(f);
    
    if (m_cal1) m_cal1->setFont(f);
    if (m_cal2) m_cal2->setFont(f);
}

#include "tqtdateperiodpickerpopup.moc"
