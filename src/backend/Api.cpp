// Pegasus Frontend
// Copyright (C) 2017  Mátyás Mustoha
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

#include "DataFinder.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrent>


ApiObject::ApiObject(QObject* parent)
    : QObject(parent)
    , m_current_platform_idx(-1)
    , m_current_platform(nullptr)
{
    QFuture<void> future = QtConcurrent::run([this]{
        QElapsedTimer timer;
        timer.start();

        this->m_platforms = DataFinder::find();

        this->m_meta.setElapsedLoadingTime(timer.elapsed());
    });

    m_loading_watcher.setFuture(future);
    connect(&m_loading_watcher, &QFutureWatcher<void>::finished,
            this, &ApiObject::onLoadingFinished);

    // subcomponent signals
    connect(&m_settings, &ApiParts::Settings::languageChanged,
            this, &ApiObject::languageChanged);
}

void ApiObject::onLoadingFinished()
{
    for (int i = 0; i < m_platforms.length(); i++) {
        Model::Platform* platform = m_platforms.at(i);
        Q_ASSERT(platform);

        connect(platform, &Model::Platform::currentGameChanged,
                [this, i](){ ApiObject::onPlatformGameChanged(i); });


        // set the initial game indices
        // empty platforms should have been removed previously
        Q_ASSERT(platform->m_games.length() > 0);
        platform->setCurrentGameIndex(0);
    }

    emit platformModelChanged();

    if (!m_platforms.isEmpty())
        setCurrentPlatformIndex(0);

    m_meta.onApiLoadingFinished();
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

    for (Model::Platform* platform : qAsConst(m_platforms))
        platform->resetGameIndex();
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
    emit currentGameChanged();
}

void ApiObject::launchGame()
{
    if (!m_current_platform) {
        qWarning() << tr("The current platform is undefined, you can't launch any games!");
        return;
    }
    if (!m_current_platform->m_current_game) {
        qWarning() << tr("The current game is undefined, you can't launch it!");
        return;
    }

    emit prepareLaunch();
}

void ApiObject::onReadyToLaunch()
{
    Q_ASSERT(m_current_platform);
    Q_ASSERT(m_current_platform->m_current_game);

    emit executeLaunch(m_current_platform, m_current_platform->m_current_game);
}

void ApiObject::onGameFinished()
{
    // TODO: this is where play count could be increased

    emit restoreAfterGame(this);
}

void ApiObject::onPlatformGameChanged(int platformIndex)
{
    if (platformIndex == m_current_platform_idx)
        emit currentGameChanged();
}
