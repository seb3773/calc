#include "tqtdateperiodpickercalendarpanel.h"

#include "tqtdateperiodpickernavigator.h"
#include "tqtdateperiodpickermonthview.h"
#include "tqtdateperiodpickeryearview.h"
#include "tqtdateperiodpickerdecadeview.h"

#include <ntqlayout.h>
#include <ntqwidgetstack.h>
#include <ntqframe.h>

static inline TQDate dppMinDateFallback_()
{
    return TQDate(1752, 9, 14);
}

TQtDatePeriodPickerCalendarPanel::TQtDatePeriodPickerCalendarPanel(TQWidget* parent)
    : TQWidget(parent),
      m_nav(0),
      m_stack(0),
      m_line(0),
      m_month(0),
      m_year(0),
      m_decade(0),
      m_periodShown(0),
      m_periodBeginShown(0),
      m_begin(),
      m_end(),
      m_min(),
      m_max()
{
    m_nav = new TQtDatePeriodPickerNavigator(this);

    m_stack = new TQWidgetStack(this);

    m_month = new TQtDatePeriodPickerMonthView(m_stack);
    m_year = new TQtDatePeriodPickerYearView(m_stack);
    m_decade = new TQtDatePeriodPickerDecadeView(m_stack);

    m_stack->addWidget(m_month, TQtDPPMonthView);
    m_stack->addWidget(m_year, TQtDPPYearView);
    m_stack->addWidget(m_decade, TQtDPPDecadeView);

    m_stack->raiseWidget(TQtDPPMonthView);

    m_line = new TQFrame(this);
    m_line->setFrameStyle(TQFrame::HLine | TQFrame::Plain);

    TQVBoxLayout* lay = new TQVBoxLayout(this);
    lay->setMargin(0);
    lay->setSpacing(1);
    lay->addWidget(m_nav);
    lay->addWidget(m_stack, 1);
    lay->addWidget(m_line);

    m_nav->setView(TQtDPPMonthView);
    m_nav->setDate(m_month->selectedDate());

    connect(m_nav, SIGNAL(viewChanged(int)), this, SLOT(setView(int)));
    connect(m_nav, SIGNAL(toPrevious()), this, SLOT(previous()));
    connect(m_nav, SIGNAL(toNext()), this, SLOT(next()));

    connect(m_month, SIGNAL(dateClicked(const TQDate&)), this, SIGNAL(dateSelected(const TQDate&)));
    connect(m_month, SIGNAL(currentPageChanged(int,int)), this, SLOT(onCurrentMonthChanged_(int,int)));

    connect(m_year, SIGNAL(monthClicked(int)), this, SLOT(onYearMonthClicked_(int)));
    connect(m_decade, SIGNAL(yearClicked(int)), this, SLOT(onDecadeYearClicked_(int)));

    setMinimumDate(TQDate());
    setMaximumDate(TQDate());

    setDate(TQDate::currentDate());
}

TQtDatePeriodPickerCalendarPanel::~TQtDatePeriodPickerCalendarPanel() {}

TQDate TQtDatePeriodPickerCalendarPanel::selectedDate() const
{
    return m_month ? m_month->selectedDate() : TQDate();
}

void TQtDatePeriodPickerCalendarPanel::setFrameVisible(int on)
{
    if (m_line) m_line->setShown(on ? true : false);
}

void TQtDatePeriodPickerCalendarPanel::setView(int view)
{
    if (m_nav) m_nav->setView(view);
    if (m_stack) m_stack->raiseWidget(view);
}

void TQtDatePeriodPickerCalendarPanel::setMinimumDate(const TQDate& date)
{
    m_min = date;
    if (m_month) m_month->setMinimumDate(date.isValid() ? date : dppMinDateFallback_());
    if (m_year) m_year->setMinimumDate(date);
    if (m_decade) m_decade->setMinimumDate(date);
}

void TQtDatePeriodPickerCalendarPanel::setMaximumDate(const TQDate& date)
{
    m_max = date;
    if (m_month) m_month->setMaximumDate(date.isValid() ? date : TQDate(7999, 12, 31));
    if (m_year) m_year->setMaximumDate(date);
    if (m_decade) m_decade->setMaximumDate(date);
}

void TQtDatePeriodPickerCalendarPanel::limitMinimum_(const TQDate& minDate)
{
    if (m_month) m_month->setMinimumDate(minDate.isValid() ? minDate : dppMinDateFallback_());
    if (m_year) m_year->setMinimumDate(minDate);
    if (m_decade) m_decade->setMinimumDate(minDate);
}

void TQtDatePeriodPickerCalendarPanel::limitMaximum_(const TQDate& maxDate)
{
    if (m_month) m_month->setMaximumDate(maxDate.isValid() ? maxDate : TQDate(7999, 12, 31));
    if (m_year) m_year->setMaximumDate(maxDate);
    if (m_decade) m_decade->setMaximumDate(maxDate);
}

void TQtDatePeriodPickerCalendarPanel::setFont(const TQFont& font)
{
    TQWidget::setFont(font);
    if (m_nav) m_nav->setFont(font);
    if (m_month) m_month->setFont(font);
    if (m_year) m_year->setFont(font);
    if (m_decade) m_decade->setFont(font);
}

void TQtDatePeriodPickerCalendarPanel::setDate(const TQDate& date)
{
    if (m_nav) m_nav->setDate(date);

    if (m_month) m_month->setDate(date);
    if (m_year) m_year->setDate(date);
    if (m_decade) m_decade->setDate(date);

    m_begin = date;
    m_end = date;
    m_periodShown = 0;
    m_periodBeginShown = 0;

    emit scrolledTo(date);
}

void TQtDatePeriodPickerCalendarPanel::setCurrentPage(int year, int month)
{
    if (month < 1) month = 1;
    if (month > 12) month = 12;

    TQDate base = (m_nav ? m_nav->date() : TQDate::currentDate());
    int day = base.day();

    const int dim = TQDate(year, month, 1).daysInMonth();
    if (day < 1) day = 1;
    if (day > dim) day = dim;

    const TQDate d(year, month, day);

    if (m_nav) m_nav->setDate(d);
    if (m_year) m_year->setDate(d);
    if (m_decade) m_decade->setDate(d);

    if (m_month) m_month->setCurrentPage(year, month);

    emit scrolledTo(d);
}

void TQtDatePeriodPickerCalendarPanel::setPeriod(const TQDate& begin, const TQDate& end, int showsBegin)
{
    const TQDate shown = showsBegin ? begin : end;

    if (m_nav) m_nav->setDate(shown);

    if (m_month) m_month->setPeriod(begin, end, showsBegin);
    if (m_year) m_year->setDate(shown);
    if (m_decade) m_decade->setDate(shown);

    m_begin = begin;
    m_end = end;
    m_periodShown = 1;
    m_periodBeginShown = showsBegin ? 1 : 0;

    if (m_periodBeginShown) {
        limitMinimum_(m_min);
        limitMaximum_(end);
    } else {
        limitMinimum_(begin);
        limitMaximum_(m_max);
    }

    emit scrolledTo(shown);
}

void TQtDatePeriodPickerCalendarPanel::scroll_(int dir)
{
    if (!m_nav) return;

    const int v = (int)m_nav->view();
    if (v == TQtDPPMonthView) {
        if (m_periodShown) {
            if (m_periodBeginShown) setPeriod(m_begin.addMonths(1 * dir), m_end, m_periodBeginShown);
            else setPeriod(m_begin, m_end.addMonths(1 * dir), m_periodBeginShown);
        } else {
            setDate(m_begin.addMonths(1 * dir));
        }
    } else if (v == TQtDPPYearView) {
        if (m_periodShown) {
            if (m_periodBeginShown) setPeriod(m_begin.addYears(1 * dir), m_end, m_periodBeginShown);
            else setPeriod(m_begin, m_end.addYears(1 * dir), m_periodBeginShown);
        } else {
            setDate(m_begin.addYears(1 * dir));
        }
    } else if (v == TQtDPPDecadeView) {
        if (m_periodShown) {
            if (m_periodBeginShown) setPeriod(m_begin.addYears(10 * dir), m_end, m_periodBeginShown);
            else setPeriod(m_begin, m_end.addYears(10 * dir), m_periodBeginShown);
        } else {
            setDate(m_begin.addYears(10 * dir));
        }
    }
}

void TQtDatePeriodPickerCalendarPanel::previous() { scroll_(-1); }
void TQtDatePeriodPickerCalendarPanel::next() { scroll_(1); }

void TQtDatePeriodPickerCalendarPanel::reset()
{
    if (m_nav) m_nav->reset();
}

void TQtDatePeriodPickerCalendarPanel::onYearMonthClicked_(int month)
{
    TQDate current = m_nav ? m_nav->date() : TQDate::currentDate();
    current = TQDate(current.year(), month, current.day());

    if (m_periodShown) {
        if (m_periodBeginShown) setPeriod(current, m_end, m_periodBeginShown);
        else setPeriod(m_begin, current, m_periodBeginShown);
    } else {
        setDate(current);
    }

    setView(TQtDPPMonthView);
}

void TQtDatePeriodPickerCalendarPanel::onDecadeYearClicked_(int year)
{
    TQDate current = m_nav ? m_nav->date() : TQDate::currentDate();
    current = TQDate(year, current.month(), current.day());

    if (m_periodShown) {
        if (m_periodBeginShown) setPeriod(current, m_end, m_periodBeginShown);
        else setPeriod(m_begin, current, m_periodBeginShown);
    } else {
        setDate(current);
    }

    setView(TQtDPPYearView);
}

void TQtDatePeriodPickerCalendarPanel::onCurrentMonthChanged_(int year, int month)
{
    TQDate current = m_nav ? m_nav->date() : TQDate::currentDate();
    int day = current.day();

    const int dim = TQDate(year, month, 1).daysInMonth();
    if (day < 1) day = 1;
    if (day > dim) day = dim;

    current = TQDate(year, month, day);

    if (m_periodShown) {
        if (m_periodBeginShown) setPeriod(current, m_end, m_periodBeginShown);
        else setPeriod(m_begin, current, m_periodBeginShown);
    } else {
        setDate(current);
    }
}

#include "tqtdateperiodpickercalendarpanel.moc"
