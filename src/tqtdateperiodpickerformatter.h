#ifndef TQTDATEPERIODPICKERFORMATTER_H
#define TQTDATEPERIODPICKERFORMATTER_H

#include <ntqstring.h>
#include <ntqdatetime.h>

class TQtDatePeriodPickerAbstractFormatter {
public:
    virtual ~TQtDatePeriodPickerAbstractFormatter();

    virtual TQString format(const TQDate& date) const = 0;
    virtual TQString format(const TQDate& begin, const TQDate& end) const = 0;
    virtual TQString format(const TQDateTime& begin, const TQDateTime& end) const = 0;
};

class TQtDatePeriodPickerStandardFormatter : public TQtDatePeriodPickerAbstractFormatter {
public:
    TQtDatePeriodPickerStandardFormatter();

    void setDateFormat(const TQString& fmt);
    void setDatePeriodFormat(const TQString& fmt);
    void setDateTimePeriodFormat(const TQString& fmt);

    virtual TQString format(const TQDate& date) const;
    virtual TQString format(const TQDate& begin, const TQDate& end) const;
    virtual TQString format(const TQDateTime& begin, const TQDateTime& end) const;

private:
    TQString m_dateFormat;
    TQString m_datePeriodFormat;
    TQString m_dateTimePeriodFormat;
};

class TQtDatePeriodPickerHumanReadableFormatter : public TQtDatePeriodPickerAbstractFormatter {
public:
    TQtDatePeriodPickerHumanReadableFormatter();

    void setDateFromWord(const TQString& word);
    void setDateToWord(const TQString& word);

    void setTimeFromWord(const TQString& word);
    void setTimeToWord(const TQString& word);

    void setInvalidDateWord(const TQString& word);
    void setInvalidPeriodWord(const TQString& word);

    void setSpecialDayWordShown(int on);

    virtual TQString format(const TQDate& date) const;
    virtual TQString format(const TQDate& begin, const TQDate& end) const;
    virtual TQString format(const TQDateTime& begin, const TQDateTime& end) const;

private:
    TQString monthName_(int month) const;

private:
    TQString m_dateFromWord;
    TQString m_dateToWord;

    TQString m_timeFromWord;
    TQString m_timeToWord;

    TQString m_invalidDateWord;
    TQString m_invalidPeriodWord;

    int m_specialDayWordShown;
};

#endif
