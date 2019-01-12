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


#pragma once

#include "GameAssetsData.h"
#include "utils/HashMap.h"
#include "utils/MoveOnly.h"

#include <QDateTime>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QVector>


namespace modeldata {

struct GameFile {
    QString name;
    // TODO: in the future...
    // QString summary;
    // QString description;
    // QString launch_cmd;
    // QString launch_workdir;

    GameFile();
    explicit GameFile(const QFileInfo&);
    MOVE_ONLY(GameFile)
};

struct Game {
    explicit Game(const QFileInfo&);
    explicit Game(QString title);
    MOVE_ONLY(Game)

    const QString title;
    QString summary;
    QString description;

    QString launch_cmd;
    QString launch_workdir;
    HashMap<QString, GameFile> files;

    int player_count;
    bool is_favorite;
    float rating;
    QDate release_date;

    int playcount;
    qint64 playtime;
    QDateTime last_played;

    QStringList developers;
    QStringList publishers;
    QStringList genres;

    GameAssets assets;
};

} // namespace modeldata
