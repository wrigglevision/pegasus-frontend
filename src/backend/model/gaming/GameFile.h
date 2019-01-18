#pragma once

#include "modeldata/gaming/GameData.h"

#include <QObject>
#include <QString>


namespace model {
class GameFile : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString path READ path CONSTANT)

public:
    explicit GameFile(modeldata::GameFile, QObject*);

    Q_INVOKABLE void launch();

    const modeldata::GameFile& data() const { return m_data; }

    const QString& name() const { return m_data.name; }
    QString path() const { return m_data.fileinfo.filePath(); }

signals:
    void launchRequested();

private:
    const modeldata::GameFile m_data;
};
} // namespace model
