#include "ParserTools.h"

#include "LocaleUtils.h"


namespace {

int find_next_nonspace(const QString& line, int start)
{
    while (start < line.length() && line.at(start).isSpace())
        start++;

    return start;
}
int find_next_unescaped(const QString& line, int start, const QChar kar)
{
    while (start < line.length() && line.at(start) != kar) {
        if (line.at(start) == QLatin1Char('\\'))
            start++;

        start++;
    }
    return start;
}

QStringRef extract_next_value(const QString& line, int& start)
{
    QChar kar = line.at(start);
    QChar end_kar(' ');

    start = find_next_nonspace(line, start);
    if (start >= line.length())
        return {};

    const bool is_quoted = (kar == QLatin1Char('\'') || kar == QLatin1Char('"'));
    if (is_quoted) {
        end_kar = kar;
        start++;
    }

    const int end = std::min(line.length(), find_next_unescaped(line, start + 1, end_kar));
    int len = end - start;
    if (is_quoted && len > 1 && line.at(end) == end_kar)
        len--;

    QStringRef result = line.midRef(start, len);
    start = end;
    return result;
}

QStringRef extract_next_char(const QString& line, int& start)
{
    start = find_next_nonspace(line, start);
    if (start >= line.length())
        return {};

    QStringRef result = line.midRef(start, 1);
    start++;
    return result;
}

} // namespace



namespace utils {

TokenizerResult tokenize_file_entry(const QString& line)
{
    TokenizerResult result;
    int from = 0;

    QStringRef part = extract_next_value(line, from);
    if (part.isEmpty()) {
        result.error_msg = tr_log("no file path defined");
        return result;
    }
    result.parts.append(part);

    do {
        part = extract_next_value(line, from);
        if (part.isEmpty())
            continue;

        result.parts.append(part);

        part = extract_next_char(line, from);
        const bool sep_found = (part == QLatin1Char(':') || part == QLatin1Char('='));
        if (!sep_found) {
            result.error_msg = tr_log("expected either ':' or '=' after `%1`, but it was missing")
                .arg(result.parts.constLast());
            continue;
        }

        part = extract_next_value(line, from);
        if (part.isEmpty()) {
            result.error_msg = tr_log("value is missing after `%1`")
                .arg(result.parts.constLast());
            continue;
        }

        result.parts.append(part);
    }
    while (!part.isEmpty() && result.error_msg.isEmpty());

    return result;
}

TokenizerResult tokenize_comma_list(const QString& line)
{
    TokenizerResult result;
    int from = 0;

    QStringRef part = extract_next_value(line, from);
    if (part.isEmpty()) {
        result.error_msg = tr_log("no items defined");
        return result;
    }
    result.parts.append(part);

    do {
        part = extract_next_char(line, from);
        if (part.isEmpty())
            continue;

        if (part != QLatin1Char(',')) {
            result.error_msg = tr_log("expected ',' after `%1`, but it was missing")
                .arg(result.parts.constLast());
            continue;
        }

        part = extract_next_value(line, from);
        if (!part.isEmpty())
            result.parts.append(part);
    }
    while (!part.isEmpty() && result.error_msg.isEmpty());

    return result;
}

} // namespace utils
