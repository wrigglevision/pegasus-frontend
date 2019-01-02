// Pegasus Frontend
// Copyright (C) 2017-2018  Mátyás Mustoha
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.


#include "Api.h"

#include "LocaleUtils.h"


ApiObject::ApiObject(QObject* parent)
    : QObject(parent)
    , m_launch_game(nullptr)
    , m_providerman(this)
{
    connect(&m_memory, &model::Memory::dataChanged,
            this, &ApiObject::memoryChanged);

    connect(&m_internal.settings().locales(), &model::Locales::localeChanged,
            this, &ApiObject::localeChanged);
    connect(&m_internal.settings().keyEditor(), &model::KeyEditor::keysChanged,
            &m_keys, &model::Keys::refresh_keys);
    connect(&m_internal.settings().themes(), &model::Themes::themeChanged,
            this, &ApiObject::onThemeChanged);

    connect(&m_providerman, &ProviderManager::gameCountChanged,
            &m_internal.meta(), &model::Meta::onGameCountUpdate);
    connect(&m_providerman, &ProviderManager::firstPhaseComplete,
            &m_internal.meta(), &model::Meta::onFirstPhaseCompleted);
    connect(&m_providerman, &ProviderManager::secondPhaseComplete,
            &m_internal.meta(), &model::Meta::onSecondPhaseCompleted);
    connect(&m_providerman, &ProviderManager::staticDataReady,
            this, &ApiObject::onStaticDataLoaded);

    onThemeChanged();
}

void ApiObject::startScanning()
{
    m_providerman.startSearch(m_allGames, m_collections);
}

void ApiObject::onStaticDataLoaded()
{
    qInfo().noquote() << tr_log("%1 games found").arg(m_allGames.count());

    for (model::Game* game : m_allGames) {
        connect(game, &model::Game::launchRequested,
                this, &ApiObject::onGameLaunchRequested);
        connect(game, &model::Game::favoriteChanged,
                this, &ApiObject::onGameFavoriteChanged);
    }

    m_internal.meta().onUiReady();
}

void ApiObject::onGameLaunchRequested()
{
    // avoid launch spamming
    if (m_launch_game)
        return;

    m_launch_game = static_cast<model::Game*>(QObject::sender());
    emit launchGame(m_launch_game);
}

void ApiObject::onGameLaunchOk()
{
    Q_ASSERT(m_launch_game);
    m_providerman.onGameLaunched(m_launch_game);
}

void ApiObject::onGameLaunchError()
{
    // TODO: show error
    Q_ASSERT(m_launch_game);
    m_launch_game = nullptr;
}

void ApiObject::onGameFinished()
{
    Q_ASSERT(m_launch_game);

    m_providerman.onGameFinished(m_launch_game);
    m_launch_game = nullptr;
}

void ApiObject::onGameFavoriteChanged()
{
    m_providerman.onGameFavoriteChanged(m_allGames.asList());
}

void ApiObject::onThemeChanged()
{
    m_memory.changeTheme(m_internal.settings().themes().currentQmlDir());
}
