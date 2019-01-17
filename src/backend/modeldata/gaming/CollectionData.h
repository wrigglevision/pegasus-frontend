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
#include "utils/MoveOnly.h"

#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <vector>


namespace modeldata {

struct Game;

struct Collection {
    explicit Collection(QString name);
    MOVE_ONLY(Collection)

    QString launch_cmd; // TODO: remove
    QString launch_workdir; // TODO: remove

    const QString name;
    QString summary;
    QString description;

    GameAssets assets; // TODO: remove

    const QString& shortName() const { return m_short_name; }
    void setShortName(const QString&);

private:
    QString m_short_name;
};

} // namespace modeldata
