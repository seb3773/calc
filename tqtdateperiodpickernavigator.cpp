#include "tqtdateperiodpickernavigator.h"

#include <ntqlayout.h>
#include <ntqpushbutton.h>

TQtDatePeriodPickerNavigator::TQtDatePeriodPickerNavigator(TQWidget* parent)
    : TQWidget(parent),
      m_view(TQtDPPMonthView),
      m_date(TQDate::currentDate()),
      m_prev(0),
      m_next(0),
      m_current(0)
{
    m_prev = new TQPushButton("<", this);
    m_prev->setFixedSize(20, 20);
    m_prev->setFlat(true);
    m_prev->setFocusPolicy(NoFocus);

    m_next = new TQPushButton(">", this);
    m_next->setFixedSize(20, 20);
    m_next->setFlat(true);
    m_next->setFocusPolicy(NoFocus);

    m_current = new TQPushButton(this);
    m_current->setFixedHeight(20);
    m_current->setFlat(true);
    m_current->setFocusPolicy(NoFocus);

    TQFont f = font();
    f.setBold(true);
    setFont(f);

    TQHBoxLayout* lay = new TQHBoxLayout(this);
    lay->setMargin(0);
    lay->setSpacing(0);
    lay->addWidget(m_prev);
    lay->addWidget(m_current, 1);
    lay->addWidget(m_next);

    connect(m_prev, SIGNAL(clicked()), this, SLOT(onPrev_()));
    connect(m_next, SIGNAL(clicked()), this, SLOT(onNext_()));
    connect(m_current, SIGNAL(clicked()), this, SLOT(onCurrent_()));

    updateTitle_();
}

TQtDatePeriodPickerNavigator::~TQtDatePeriodPickerNavigator() {}

TQDate TQtDatePeriodPickerNavigator::date() const { return m_date; }
TQtDatePeriodPickerView TQtDatePeriodPickerNavigator::view() const { return (TQtDatePeriodPickerView)m_view; }

void TQtDatePeriodPickerNavigator::setFont(const TQFont& font)
{
    TQWidget::setFont(font);
    TQFontMetrics fm(font);
    int h = fm.height();
    int btnSize = h + 12;

    if (m_prev) {
        m_prev->setFixedSize(btnSize, btnSize);
        m_prev->setFont(font);
    }
    if (m_next) {
        m_next->setFixedSize(btnSize, btnSize);
        m_next->setFont(font);
    }
    if (m_current) {
        m_current->setFixedHeight(btnSize);
        TQFont boldFont = font;
        boldFont.setBold(true);
        m_current->setFont(boldFont);
    }
}

void TQtDatePeriodPickerNavigator::setView(int view)
{
    if (m_view == view) return;
    m_view = view;
    updateTitle_();
    emit viewChanged(m_view);
}

void TQtDatePeriodPickerNavigator::setDate(const TQDate& date)
{
    m_date = date;
    updateTitle_();
}

void TQtDatePeriodPickerNavigator::reset()
{
    setView(TQtDPPMonthView);
}

void TQtDatePeriodPickerNavigator::onPrev_() { emit toPrevious(); }
void TQtDatePeriodPickerNavigator::onNext_() { emit toNext(); }

void TQtDatePeriodPickerNavigator::onCurrent_()
{
    if (m_view == TQtDPPMonthView) setView(TQtDPPYearView);
    else if (m_view == TQtDPPYearView) setView(TQtDPPDecadeView);
}

void TQtDatePeriodPickerNavigator::updateTitle_()
{
    TQString t;

    if (m_view == TQtDPPMonthView) {
        t = m_date.toString("MMMM yyyy");
    } else if (m_view == TQtDPPYearView) {
        t = m_date.toString("yyyy");
    } else if (m_view == TQtDPPDecadeView) {
        t = m_date.addYears(-4).toString("yyyy") + TQString(" - ") + m_date.addYears(5).toString("yyyy");
    }

    if (m_current) m_current->setText(t);
}

#include "tqtdateperiodpickernavigator.moc"
