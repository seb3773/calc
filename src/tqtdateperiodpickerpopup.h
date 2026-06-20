#ifndef TQTDATEPERIODPICKER_POPUP_H
#define TQTDATEPERIODPICKER_POPUP_H

#include <ntqwidget.h>
#include <ntqdatetime.h>

#include "tqtdateperiodpicker_common.h"

class TQtDatePeriodPickerCalendarPanel;
class TQtDatePeriodPickerPopupFooter;
class TQTimeEdit;

class TQtDatePeriodPickerPopup : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPickerPopup(TQWidget* parent = 0);
    ~TQtDatePeriodPickerPopup();

    int isTimeEditable() const;
    unsigned int timeDisplay() const;

    int datePickerType() const;
    TQtDatePeriodPickerTypes allowedPickerTypes() const;

    TQDate date() const;
    TQDate periodBegin() const;
    TQDate periodEnd() const;

    TQTime timeBegin() const;
    TQTime timeEnd() const;

signals:
    void dateSelected(const TQDate& date);
    void datePeriodSelected(const TQDate& begin, const TQDate& end);
    void timePeriodSelected(const TQTime& begin, const TQTime& end);

    void datePickerTypeChanged(int type);

    void accepted();

public slots:
    void setMinimumDate(const TQDate& date);
    void setMaximumDate(const TQDate& date);

    void setDate(const TQDate& date);
    void setDatePeriod(const TQDate& begin, const TQDate& end);

    void setTimePeriod(const TQTime& begin, const TQTime& end);

    void setDatePickerType(int pickerType);
    void setAllowedPickerTypes(TQtDatePeriodPickerTypes pickerTypes);

    void setTimeEditable(int on);
    void setTimeDisplay(unsigned int disp);

    void reset();

    void accept();

protected:
    void paintEvent(TQPaintEvent* ev);
    void resizeEvent(TQResizeEvent* ev);

private slots:
    void onCalendar1DateSelected_(const TQDate& date);
    void onCalendar2DateSelected_(const TQDate& date);

    void onCalendar1Scrolled_(const TQDate& date);
    void onCalendar2Scrolled_(const TQDate& date);

    void onTimeEdit1TimeChanged_(const TQTime& time);
    void onTimeEdit2TimeChanged_(const TQTime& time);

private:
    void setDateInternal_(const TQDate& date);
    void setDatePeriodInternal_(const TQDate& begin, const TQDate& end);
    void setTimePeriodInternal_(const TQTime& begin, const TQTime& end);

    void updateTypeUi_();
    void updateTimeUi_();
    void updateSecondCalendarMonth_();

private:
    int m_syncingScroll;

    int m_timeEditable;
    int m_pickerType;
    TQtDatePeriodPickerTypes m_allowedTypes;

    TQDate m_begin;
    TQDate m_end;

    TQTime m_timeBegin;
    TQTime m_timeEnd;

    TQtDatePeriodPickerCalendarPanel* m_cal1;
    TQtDatePeriodPickerCalendarPanel* m_cal2;

    TQTimeEdit* m_te1;
    TQTimeEdit* m_te2;

    TQtDatePeriodPickerPopupFooter* m_footer;
};

#endif
