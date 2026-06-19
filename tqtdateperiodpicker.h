#ifndef TQTDATEPERIODPICKER_H
#define TQTDATEPERIODPICKER_H

#include <ntqwidget.h>
#include <ntqdatetime.h>

#include "tqtdateperiodpicker_common.h"

class TQLabel;
class CalcButton;
class TQtDatePeriodPickerPopup;
class TQtDatePeriodPickerAbstractFormatter;

class TQtDatePeriodPicker : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPicker(TQWidget* parent = 0);
    ~TQtDatePeriodPicker();

    int isEditable() const;

    int datePickerType() const;
    TQtDatePeriodPickerTypes allowedPickerTypes() const;

    int isTimeEditable() const;

    const TQtDatePeriodPickerAbstractFormatter* formatter() const;

    TQDate date() const;
    TQDate periodBegin() const;
    TQDate periodEnd() const;

    TQTime timeBegin() const;
    TQTime timeEnd() const;

    TQDateTime dateTimePeriodBegin() const;
    TQDateTime dateTimePeriodEnd() const;

signals:
    void editingFinished();

    void dateChanged(const TQDate& date);
    void datePeriodChanged(const TQDate& begin, const TQDate& end);
    void timePeriodChanged(const TQTime& begin, const TQTime& end);
    void dateTimePeriodChanged(const TQDateTime& begin, const TQDateTime& end);

    void datePickerTypeChanged(int type);

public slots:
    void setEditable(int on);

    void setDatePickerType(int type);
    void setAllowedPickerTypes(TQtDatePeriodPickerTypes types);

    void setTimeEditable(int on);

    void setFormatter(TQtDatePeriodPickerAbstractFormatter* formatter);

    void setDate(const TQDate& date);
    void setDatePeriod(const TQDate& begin, const TQDate& end);

    void setTimePeriod(const TQTime& begin, const TQTime& end);
    void setDateTimePeriod(const TQDateTime& begin, const TQDateTime& end);

private slots:
    void onShowPopup_();

    void onPopupDateSelected_(const TQDate& date);
    void onPopupDatePeriodSelected_(const TQDate& begin, const TQDate& end);
    void onPopupTimePeriodSelected_(const TQTime& begin, const TQTime& end);

protected:
    bool eventFilter(TQObject* obj, TQEvent* ev);
    void showEvent(TQShowEvent* ev);

public:
    void setButtonSize(int size);

private:
    void updateLabel_();
    void adjustPopupPosition_();

private:
    int m_editable;

    int m_pickerType;
    TQtDatePeriodPickerTypes m_allowed;

    int m_timeEditable;

    TQDate m_dateBegin;
    TQDate m_dateEnd;

    TQTime m_timeBegin;
    TQTime m_timeEnd;

    TQLabel* m_label;
    CalcButton* m_btn;
    TQtDatePeriodPickerPopup* m_popup;

    TQtDatePeriodPickerAbstractFormatter* m_formatter;
};

#endif
