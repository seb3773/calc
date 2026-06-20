#ifndef TQTDATEPERIODPICKER_POPUPFOOTER_H
#define TQTDATEPERIODPICKER_POPUPFOOTER_H

#include <ntqwidget.h>

#include "tqtdateperiodpicker_common.h"

class TQRadioButton;

class TQtDatePeriodPickerPopupFooter : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPickerPopupFooter(TQWidget* parent = 0);
    ~TQtDatePeriodPickerPopupFooter();

    TQtDatePeriodPickerType pickerType() const;

public slots:
    void setAllowedPickerTypes(TQtDatePeriodPickerTypes types);
    void setPickerType(int type);

signals:
    void pickerTypeChanged(int type);
    void okClicked();

private slots:
    void onDayToggled_(bool on);
    void onPeriodToggled_(bool on);
    void onOk_();

private:
    void enforceAllowed_();

private:
    TQtDatePeriodPickerTypes m_allowed;

    TQRadioButton* m_day;
    TQRadioButton* m_period;

    class TQPushButton* m_ok;
};

#endif
