#include "Api.h"

#include "Es2Systems.h"

#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>


ApiObject::ApiObject(QObject* parent)
    : QObject(parent)
    , m_current_platform_idx(-1)
    , m_current_game_idx(-1)
    , m_current_platform(nullptr)
    , m_current_game(nullptr)
{
    Es2::Systems::read(m_platforms); // TODO: check result
    if (!m_platforms.isEmpty())
        setCurrentPlatformIndex(0);
}

QQmlListProperty<Model::Platform> ApiObject::getPlatformsProp()
{
    return QQmlListProperty<Model::Platform>(this, m_platforms);
}

void ApiObject::resetPlatformIndex()
{
    // these values are always in pair
    Q_ASSERT((m_current_platform_idx == -1) == (m_current_platform == nullptr));
    if (!m_current_platform) // already reset
        return;

    m_current_platform_idx = -1;
    m_current_platform = nullptr;
    emit currentPlatformChanged();

    resetGameIndex();
}

void ApiObject::resetGameIndex()
{
    // these values are always in pair
    Q_ASSERT((m_current_game_idx == -1) == (m_current_game == nullptr));
    if (!m_current_game) // already reset
        return;

    m_current_game_idx = -1;
    m_current_game = nullptr;
    emit currentGameChanged();
}

void ApiObject::setCurrentPlatformIndex(int idx)
{
    if (idx == m_current_platform_idx)
        return;

    if (idx == -1) {
        resetPlatformIndex();
        return;
    }

    const bool valid_idx = (0 <= idx || idx < m_platforms.count());
    if (!valid_idx) {
        qWarning() << tr("Invalid platform index #%1").arg(idx);
        return;
    }

    m_current_platform_idx = idx;
    m_current_platform = m_platforms.at(idx);
    Q_ASSERT(m_current_platform);
    emit currentPlatformChanged();

    if (m_platforms.at(idx)->m_games.isEmpty())
        resetGameIndex();
    else
        setCurrentGameIndex(0);
}

void ApiObject::setCurrentGameIndex(int idx)
{
    if (idx == -1) {
        resetGameIndex();
        return;
    }

    if (!m_current_platform) {
        qWarning() << tr("Could not set game index, the current platform is undefined!");
        return;
    }

    const bool valid_idx = (0 <= idx || idx < m_current_platform->m_games.count());
    if (!valid_idx) {
        qWarning() << tr("Invalid game index #%1").arg(idx);
        return;
    }

    Model::Game* new_game = m_current_platform->m_games.at(idx);
    if (m_current_game == new_game)
        return;

    m_current_game_idx = idx;
    m_current_game = new_game;
    Q_ASSERT(m_current_game);
    emit currentGameChanged();
}

void ApiObject::launchGame()
{
    if (!m_current_platform) {
        qWarning() << tr("The current platform is undefined, you can't launch any games!");
        return;
    }
    if (!m_current_game) {
        qWarning() << tr("The current game is undefined, you can't launch it!");
        return;
    }

    emit requestLaunch();
}

void ApiObject::onReadyToLaunch()
{
    const QString rom_path_basename = QFileInfo(m_current_game->m_rom_path).completeBaseName();
    QString rom_path_escaped = m_current_game->m_rom_path;
#ifdef _WIN32
    rom_path_escaped = '"' + rom_path_escaped + '"';
#else
    // based on the source code of Bash
    static constexpr auto SPECIALCHARS = R"(([\t\n !"$&'()*,;<>?\[\b\]^`{|}#~:=]))";
    rom_path_escaped.replace(QRegularExpression(SPECIALCHARS), R"(\\1)");
#endif

    QString launch_cmd = m_current_platform->m_launch_cmd;
    launch_cmd
        .replace("%ROM%", rom_path_escaped)
        .replace("%ROM_RAW%", m_current_game->m_rom_path)
        .replace("%BASENAME%", rom_path_basename);

    emit executeCommand(this, launch_cmd);
}