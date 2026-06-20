#include "tqtdateperiodpickerpopupfooter.h"

#include <ntqlayout.h>
#include <ntqpushbutton.h>
#include <ntqradiobutton.h>

TQtDatePeriodPickerPopupFooter::TQtDatePeriodPickerPopupFooter(TQWidget* parent)
    : TQWidget(parent),
      m_allowed(TQtDPPDayType | TQtDPPPeriodType),
      m_day(0),
      m_period(0),
      m_ok(0)
{
    m_day = new TQRadioButton("Day", this);
    m_period = new TQRadioButton("Range", this);

    m_ok = new TQPushButton("OK", this);
    m_ok->setFocusPolicy(NoFocus);

    TQHBoxLayout* lay = new TQHBoxLayout(this);
    lay->setMargin(0);
    lay->setSpacing(8);
    lay->addWidget(m_day);
    lay->addWidget(m_period);
    lay->addStretch(1);
    lay->addWidget(m_ok);

    connect(m_day, SIGNAL(toggled(bool)), this, SLOT(onDayToggled_(bool)));
    connect(m_period, SIGNAL(toggled(bool)), this, SLOT(onPeriodToggled_(bool)));
    connect(m_ok, SIGNAL(clicked()), this, SLOT(onOk_()));

    m_day->setChecked(true);
    enforceAllowed_();
}

TQtDatePeriodPickerPopupFooter::~TQtDatePeriodPickerPopupFooter() {}

TQtDatePeriodPickerType TQtDatePeriodPickerPopupFooter::pickerType() const
{
    if (m_period && m_period->isChecked()) return TQtDPPPeriodType;
    return TQtDPPDayType;
}

void TQtDatePeriodPickerPopupFooter::setAllowedPickerTypes(TQtDatePeriodPickerTypes types)
{
    m_allowed = types;
    enforceAllowed_();
}

void TQtDatePeriodPickerPopupFooter::setPickerType(int type)
{
    if (type == TQtDPPPeriodType) {
        if (m_period) m_period->setChecked(true);
    } else {
        if (m_day) m_day->setChecked(true);
    }
    enforceAllowed_();
}

void TQtDatePeriodPickerPopupFooter::enforceAllowed_()
{
    const int allowDay = (m_allowed & TQtDPPDayType) ? 1 : 0;
    const int allowPeriod = (m_allowed & TQtDPPPeriodType) ? 1 : 0;

    if (m_day) m_day->setEnabled(allowDay ? true : false);
    if (m_period) m_period->setEnabled(allowPeriod ? true : false);

    if (!allowDay && allowPeriod) {
        if (m_period) m_period->setChecked(true);
    }
    if (!allowPeriod && allowDay) {
        if (m_day) m_day->setChecked(true);
    }
}

void TQtDatePeriodPickerPopupFooter::onDayToggled_(bool on)
{
    if (!on) return;
    if (!(m_allowed & TQtDPPDayType)) {
        enforceAllowed_();
        return;
    }
    emit pickerTypeChanged((int)TQtDPPDayType);
}

void TQtDatePeriodPickerPopupFooter::onPeriodToggled_(bool on)
{
    if (!on) return;
    if (!(m_allowed & TQtDPPPeriodType)) {
        enforceAllowed_();
        return;
    }
    emit pickerTypeChanged((int)TQtDPPPeriodType);
}

void TQtDatePeriodPickerPopupFooter::onOk_()
{
    emit okClicked();
}

#include "tqtdateperiodpickerpopupfooter.moc"
