#ifndef TQTDATEPERIODPICKER_CALENDARPANEL_H
#define TQTDATEPERIODPICKER_CALENDARPANEL_H

#include <ntqwidget.h>
#include <ntqdatetime.h>

#include "tqtdateperiodpicker_common.h"

class TQtDatePeriodPickerNavigator;
class TQtDatePeriodPickerMonthView;
class TQtDatePeriodPickerYearView;
class TQtDatePeriodPickerDecadeView;
class TQWidgetStack;
class TQFrame;

class TQtDatePeriodPickerCalendarPanel : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPickerCalendarPanel(TQWidget* parent = 0);
    ~TQtDatePeriodPickerCalendarPanel();

    TQDate selectedDate() const;
    virtual void setFont(const TQFont& font);

signals:
    void dateSelected(const TQDate& date);
    void scrolledTo(const TQDate& date);

public slots:
    void setFrameVisible(int on);

    void setView(int view);

    void setMinimumDate(const TQDate& date);
    void setMaximumDate(const TQDate& date);

    void setDate(const TQDate& date);
    void setPeriod(const TQDate& begin, const TQDate& end, int showsBegin);

    void setCurrentPage(int year, int month);

    void previous();
    void next();

    void reset();

private slots:
    void onYearMonthClicked_(int month);
    void onDecadeYearClicked_(int year);
    void onCurrentMonthChanged_(int year, int month);

private:
    void scroll_(int dir);
    void limitMinimum_(const TQDate& minDate);
    void limitMaximum_(const TQDate& maxDate);

private:
    TQtDatePeriodPickerNavigator* m_nav;
    TQWidgetStack* m_stack;
    TQFrame* m_line;

    TQtDatePeriodPickerMonthView* m_month;
    TQtDatePeriodPickerYearView* m_year;
    TQtDatePeriodPickerDecadeView* m_decade;

    int m_periodShown;
    int m_periodBeginShown;

    TQDate m_begin;
    TQDate m_end;

    TQDate m_min;
    TQDate m_max;
};

#endif
