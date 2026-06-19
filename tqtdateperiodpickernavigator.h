#ifndef TQTDATEPERIODPICKER_NAVIGATOR_H
#define TQTDATEPERIODPICKER_NAVIGATOR_H

#include <ntqwidget.h>
#include <ntqdatetime.h>

#include "tqtdateperiodpicker_common.h"

class TQtDatePeriodPickerNavigator : public TQWidget {
    TQ_OBJECT
public:
    TQtDatePeriodPickerNavigator(TQWidget* parent = 0);
    ~TQtDatePeriodPickerNavigator();

    TQDate date() const;
    TQtDatePeriodPickerView view() const;
    virtual void setFont(const TQFont& font);

signals:
    void viewChanged(int view);
    void toPrevious();
    void toNext();

public slots:
    void setView(int view);
    void setDate(const TQDate& date);
    void reset();

private slots:
    void onPrev_();
    void onNext_();
    void onCurrent_();

private:
    void updateTitle_();

private:
    int m_view;
    TQDate m_date;

    class TQPushButton* m_prev;
    class TQPushButton* m_next;
    class TQPushButton* m_current;
};

#endif
