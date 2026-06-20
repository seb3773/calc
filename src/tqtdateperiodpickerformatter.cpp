#include "tqtdateperiodpickerformatter.h"

#include <ntqmap.h>

TQtDatePeriodPickerAbstractFormatter::~TQtDatePeriodPickerAbstractFormatter() {}

TQtDatePeriodPickerStandardFormatter::TQtDatePeriodPickerStandardFormatter()
    : m_dateFormat("yyyy-MM-dd"),
      m_datePeriodFormat("yyyy-MM-dd"),
      m_dateTimePeriodFormat("yyyy-MM-dd hh:mm:ss")
{
}

void TQtDatePeriodPickerStandardFormatter::setDateFormat(const TQString& fmt) { m_dateFormat = fmt; }
void TQtDatePeriodPickerStandardFormatter::setDatePeriodFormat(const TQString& fmt) { m_datePeriodFormat = fmt; }
void TQtDatePeriodPickerStandardFormatter::setDateTimePeriodFormat(const TQString& fmt) { m_dateTimePeriodFormat = fmt; }

TQString TQtDatePeriodPickerStandardFormatter::format(const TQDate& date) const
{
    if (!date.isValid()) return TQString();
    return date.toString(m_dateFormat);
}

TQString TQtDatePeriodPickerStandardFormatter::format(const TQDate& begin, const TQDate& end) const
{
    if (!begin.isValid() || !end.isValid()) return TQString();
    if (begin == end) return begin.toString(m_datePeriodFormat);
    return begin.toString(m_datePeriodFormat) + TQString(" - ") + end.toString(m_datePeriodFormat);
}

TQString TQtDatePeriodPickerStandardFormatter::format(const TQDateTime& begin, const TQDateTime& end) const
{
    if (!begin.isValid() || !end.isValid()) return TQString();
    if (begin == end) return begin.toString(m_dateTimePeriodFormat);
    return begin.toString(m_dateTimePeriodFormat) + TQString(" - ") + end.toString(m_dateTimePeriodFormat);
}

TQtDatePeriodPickerHumanReadableFormatter::TQtDatePeriodPickerHumanReadableFormatter()
    : m_dateFromWord("from"),
      m_dateToWord("to"),
      m_timeFromWord("from"),
      m_timeToWord("to"),
      m_invalidDateWord("undefined date"),
      m_invalidPeriodWord("undefined period"),
      m_specialDayWordShown(1)
{
}

void TQtDatePeriodPickerHumanReadableFormatter::setDateFromWord(const TQString& word) { m_dateFromWord = word; }
void TQtDatePeriodPickerHumanReadableFormatter::setDateToWord(const TQString& word) { m_dateToWord = word; }
void TQtDatePeriodPickerHumanReadableFormatter::setTimeFromWord(const TQString& word) { m_timeFromWord = word; }
void TQtDatePeriodPickerHumanReadableFormatter::setTimeToWord(const TQString& word) { m_timeToWord = word; }
void TQtDatePeriodPickerHumanReadableFormatter::setInvalidDateWord(const TQString& word) { m_invalidDateWord = word; }
void TQtDatePeriodPickerHumanReadableFormatter::setInvalidPeriodWord(const TQString& word) { m_invalidPeriodWord = word; }
void TQtDatePeriodPickerHumanReadableFormatter::setSpecialDayWordShown(int on) { m_specialDayWordShown = on ? 1 : 0; }

TQString TQtDatePeriodPickerHumanReadableFormatter::monthName_(int month) const
{
    static const char* k[] = {
        "", "january", "february", "march", "april", "may", "june",
        "july", "august", "september", "october", "november", "december"
    };
    if ((unsigned int)month > 12u) return TQString();
    return TQString(k[month]);
}

TQString TQtDatePeriodPickerHumanReadableFormatter::format(const TQDate& date) const
{
    if (!date.isValid()) return m_invalidDateWord;

    const TQDate current = TQDate::currentDate();
    const int dayDiff = current.daysTo(date);

    if (m_specialDayWordShown) {
        if (dayDiff == 0) return TQString("today");
        if (dayDiff == -1) return TQString("yesterday");
        if (dayDiff == 1) return TQString("tomorrow");
    }

    TQString s;
    s.sprintf("%d ", date.day());
    s += monthName_(date.month());
    s += TQString(" ");
    s += TQString::number(date.year());
    return s;
}

TQString TQtDatePeriodPickerHumanReadableFormatter::format(const TQDate& begin, const TQDate& end) const
{
    if (!begin.isValid() || !end.isValid()) return m_invalidPeriodWord;
    if (begin == end) return format(begin);

    TQString beginStr;
    if (begin.year() == end.year()) {
        if (begin.month() == end.month()) {
            beginStr = TQString::number(begin.day());
        } else {
            beginStr = TQString::number(begin.day()) + TQString(" ") + monthName_(begin.month());
        }
    } else {
        beginStr = TQString::number(begin.day()) + TQString(" ") + monthName_(begin.month()) + TQString(" ") + TQString::number(begin.year());
    }

    const TQString endStr = TQString::number(end.day()) + TQString(" ") + monthName_(end.month()) + TQString(" ") + TQString::number(end.year());

    return (m_dateFromWord + TQString(" ") + beginStr + TQString(" ") + m_dateToWord + TQString(" ") + endStr).simplifyWhiteSpace();
}

TQString TQtDatePeriodPickerHumanReadableFormatter::format(const TQDateTime& begin, const TQDateTime& end) const
{
    if (!begin.isValid() || !end.isValid()) return m_invalidPeriodWord;

    const TQTime tb = begin.time();
    const TQTime te = end.time();

    const TQString beginTime = (tb.second() == 0) ? tb.toString("hh:mm") : tb.toString("hh:mm:ss");
    const TQString endTime = (te.second() == 0) ? te.toString("hh:mm") : te.toString("hh:mm:ss");

    if (begin.date() == end.date()) {
        if ((tb == TQTime(0, 0, 0)) && (te == TQTime(23, 59, 59))) return format(begin.date());

        const TQString d = TQString::number(begin.date().day()) + TQString(" ") + monthName_(begin.date().month()) + TQString(" ") + TQString::number(begin.date().year());
        return (m_timeFromWord + TQString(" ") + beginTime + TQString(" ") + m_timeToWord + TQString(" ") + endTime + TQString(" ") + d).simplifyWhiteSpace();
    }

    if ((tb == TQTime(0, 0, 0)) && (te == TQTime(23, 59, 59))) return format(begin.date(), end.date());

    TQString beginDate;
    if (begin.date().year() == end.date().year()) {
        beginDate = TQString::number(begin.date().day()) + TQString(" ") + monthName_(begin.date().month());
    } else {
        beginDate = TQString::number(begin.date().day()) + TQString(" ") + monthName_(begin.date().month()) + TQString(" ") + TQString::number(begin.date().year());
    }

    const TQString endDate = TQString::number(end.date().day()) + TQString(" ") + monthName_(end.date().month()) + TQString(" ") + TQString::number(end.date().year());

    return (m_timeFromWord + TQString(" ") + beginTime + TQString(" ") + beginDate + TQString(" ") + m_timeToWord + TQString(" ") + endTime + TQString(" ") + endDate).simplifyWhiteSpace();
}
