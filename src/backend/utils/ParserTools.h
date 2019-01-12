#pragma once

#include <QString>
#include <QVector>


namespace utils {

struct TokenizerResult {
    QVector<QStringRef> parts;
    QString error_msg;
};

TokenizerResult tokenize_file_entry(const QString&);
TokenizerResult tokenize_comma_list(const QString&);

} // namespace utils
