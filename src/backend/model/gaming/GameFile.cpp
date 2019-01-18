#include "GameFile.h"


namespace model {
GameFile::GameFile(modeldata::GameFile data, QObject* parent)
    : QObject(parent)
    , m_data(std::move(data))
{}

void GameFile::launch()
{
    emit launchRequested();
}
} // namespace model
