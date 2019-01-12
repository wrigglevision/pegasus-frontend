#include "GameFile.h"


namespace model {
GameFile::GameFile(QString path, modeldata::GameFile data, QObject* parent)
    : QObject(parent)
    , m_path(std::move(path))
    , m_data(std::move(data))
{}

void GameFile::launch()
{
    emit
}
} // namespace model
