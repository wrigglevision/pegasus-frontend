#pragma once

#include "modeldata/gaming/GameData.h"

#include <QObject>
#include <QString>


#define DATA_CONSTREF(Type, name) \
    private: Q_PROPERTY(Type name READ name CONSTANT) \
    public: const Type& name() const { return m_data.name; }

#define MEMBER_CONSTREF(Type, name) \
    private: Q_PROPERTY(Type name READ name CONSTANT) \
    public: const Type& name() const { return m_##name; }


namespace model {
class GameFile : public QObject {
    Q_OBJECT
    DATA_CONSTREF(QString, name)
    MEMBER_CONSTREF(QString, path)

public:
    explicit GameFile(QString, modeldata::GameFile, QObject*);

    Q_INVOKABLE void launch();

signals:
    void launchRequested();

private:
    const QString m_path;
    const modeldata::GameFile m_data;
};
} // namespace model

#undef DATA_CONSTREF
#undef MEMBER_CONSTREF
