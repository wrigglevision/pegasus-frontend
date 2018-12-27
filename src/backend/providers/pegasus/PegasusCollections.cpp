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


#include "PegasusCollections.h"

#include "ConfigFile.h"
#include "LocaleUtils.h"
#include "Paths.h"
#include "PegasusAssets.h"
#include "PegasusCommon.h"
#include "modeldata/gaming/CollectionData.h"
#include "modeldata/gaming/GameData.h"
#include "utils/PathCheck.h"

#include <QDebug>
#include <QDirIterator>
#include <QRegularExpression>
#include <QStringBuilder>


namespace providers {
namespace pegasus {

enum class CollAttrib : unsigned char {
    SHORT_NAME,
    DIRECTORIES,
    EXTENSIONS,
    FILES,
    REGEX,
    SHORT_DESC,
    LONG_DESC,
    LAUNCH_CMD,
    LAUNCH_WORKDIR,
};
enum class GameAttrib : unsigned char {
    FILES,
    DEVELOPERS,
    PUBLISHERS,
    GENRES,
    PLAYER_COUNT,
    SHORT_DESC,
    LONG_DESC,
    RELEASE,
    RATING,
    LAUNCH_CMD,
    LAUNCH_WORKDIR,
};

} // namespace providers
} //namespace pegasus


namespace {
static constexpr auto MSG_PREFIX = "Collections:";

using CollAttrib = providers::pegasus::CollAttrib;
using GameAttrib = providers::pegasus::GameAttrib;

struct GameFilterGroup {
    QStringList extensions;
    QStringList files;
    QString regex;
};
struct GameFilter {
    QString parent_collection;
    QStringList directories;
    GameFilterGroup include;
    GameFilterGroup exclude;
    QStringList extra;

    GameFilter(QString parent, QString base_dir)
        : parent_collection(parent)
        , directories(base_dir)
    {}
};

struct ParserContext {
    const QRegularExpression rx_asset_key(QStringLiteral(R"(^assets?\.(.+)$)"));

    QString curr_dir;
    QString curr_config_path;

    std::vector<GameFilter> filters;
    std::vector<modeldata::Game> games;

    modeldata::Collection* curr_coll = nullptr;
    modeldata::Game* curr_game = nullptr;

    std::function<void(const config::Entry&)> current_parser = nullptr;


    void print_error(const int lineno, const QString msg){
        qWarning().noquote() << MSG_PREFIX
            << tr_log("`%1`, line %2: %3")
                      .arg(curr_config_path, QString::number(lineno), msg);
    }
};

void parse_collection_attrib(ParserContext& ctx, const config::Entry& entry)
{
    if (!ctx.curr_coll) {
        ctx.print_error(entry.line, tr_log("no collection defined yet, entry ignored"));
        return;
    }

    if (entry.key.startsWith(QLatin1String("x-")))
        return;

    const auto rx_asset = ctx.rx_asset_key.match(entry.key);
    if (rx_asset.hasMatch()) {
        const QString asset_key = rx_asset.captured(1);
        const AssetType asset_type = pegasus_assets::str_to_type(asset_key);
        if (asset_type == AssetType::UNKNOWN) {
            ctx.print_error(entry.line, tr_log("unknown asset type '%1', entry ignored").arg(asset_key));
            return;
        }

        providers::pegasus::add_asset(ctx.curr_coll->default_assets, asset_type,
                                      entry.values.first(), ctx.curr_dir);
        return;
    }

    if (!key_types.count(entry.key)) {
        on_error(entry.line, tr_log("unrecognized attribute name `%3`, ignored").arg(entry.key));
        return;
    }

    GameFilter& filter = filters.back();
    GameFilterGroup& filter_group = entry.key.startsWith(QLatin1String("ignore-"))
        ? filter.exclude
        : filter.include;

    switch (key_types.at(entry.key)) {
        case CollAttrib::SHORT_NAME:
            ctx.curr_coll->setShortName(entry.values.first());
            break;
        case CollAttrib::LAUNCH_CMD:
            ctx.curr_coll->launch_cmd = config::mergeLines(entry.values);
            break;
        case CollAttrib::DIRECTORIES:
            for (const QString& value : entry.values) {
                QFileInfo finfo(value);
                if (finfo.isRelative())
                    finfo.setFile(ctx.curr_dir % '/' % value);

                filter.directories.append(finfo.canonicalFilePath());
            }
            break;
        case CollAttrib::EXTENSIONS:
            filter_group.extensions.append(providers::pegasus::tokenize(config::mergeLines(entry.values).toLower()));
            break;
        case CollAttrib::FILES:
            for (const QString& value : entry.values)
                filter_group.files.append(value);
            break;
        case CollAttrib::REGEX:
            filter_group.regex = entry.values.first();
            break;
        case CollAttrib::SHORT_DESC:
            ctx.curr_coll->summary = config::mergeLines(entry.values);
            break;
        case CollAttrib::LONG_DESC:
            ctx.curr_coll->description = config::mergeLines(entry.values);
            break;
        case CollAttrib::LAUNCH_WORKDIR:
            ctx.curr_coll->launch_workdir = entry.values.first();
            break;
    }
}

void parse_game_attrib(const config::Entry& entry)
{

}



std::vector<GameFilter> read_collections_file(const HashMap<QString, CollAttrib>& key_types,
                                              const QString& dir_path,
                                              HashMap<QString, modeldata::Collection>& collections)
{
    // reminder: sections are collection names
    // including keys: extensions, files, regex
    // excluding keys: ignore-extensions, ignore-files, ignore-regex
    // optional: name, launch, directories


    const QLatin1String COLLECTION("collection");




    const auto on_error = [&](const int lineno, const QString msg){
        qWarning().noquote() << MSG_PREFIX
            << tr_log("`%1`, line %2: %3")
                      .arg(curr_config_path, QString::number(lineno), msg);
    };
    const auto on_attribute = [&](const config::Entry& entry){
        if (entry.key == COLLECTION) {
            const QString& name = entry.values.first();

            curr_coll = nullptr;
            if (!collections.count(name))
                collections.emplace(name, modeldata::Collection(name));

            curr_coll = &collections.at(name);
            filters.emplace_back(name, dir_path);

            current_parser = parse_collection_attrib;
            return;
        }







    };


    // the actual reading

    const QStringList possible_paths {
        dir_path + QStringLiteral("/metadata.pegasus.txt"),
        dir_path + QStringLiteral("/collections.pegasus.txt"),
        dir_path + QStringLiteral("/metadata.txt"),
        dir_path + QStringLiteral("/collections.txt"),
    };
    for (const QString& path : possible_paths) {
        if (!::validFile(path))
            continue;

        qInfo().noquote() << MSG_PREFIX << tr_log("found `%1`").arg(path);

        curr_coll = nullptr;
        curr_config_path = path;
        config::readFile(path, on_attribute, on_error);

        // NOTE: Temporarily turned off while moving to the new format
        // break; // if the first file exists, don't check the other
    }

    // cleanup and return

    for (GameFilter& filter : filters) {
        filter.directories.removeDuplicates();
        filter.include.extensions.removeDuplicates();
        filter.include.files.removeDuplicates();
        filter.exclude.extensions.removeDuplicates();
        filter.exclude.files.removeDuplicates();
        filter.extra.removeDuplicates();
    }
    return filters;
}

void process_filter(const GameFilter& filter,
                    HashMap<QString, modeldata::Game>& games,
                    HashMap<QString, modeldata::Collection>& collections,
                    HashMap<QString, std::vector<QString>>& collection_childs)
{
    constexpr auto entry_filters = QDir::Files | QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot;
    constexpr auto subdir_filters = QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot;
    constexpr auto entry_flags = QDirIterator::FollowSymlinks;
    constexpr auto subdir_flags = QDirIterator::FollowSymlinks | QDirIterator::Subdirectories;

    const QRegularExpression include_regex(filter.include.regex);
    const QRegularExpression exclude_regex(filter.exclude.regex);


    for (const QString& filter_dir : filter.directories)
    {
        // find all dirs and subdirectories, but ignore 'media'
        QStringList dirs_to_check;
        {
            QDirIterator dirs_it(filter_dir, subdir_filters, subdir_flags);
            while (dirs_it.hasNext())
                dirs_to_check << dirs_it.next();

            dirs_to_check.removeOne(filter_dir + QStringLiteral("/media"));
            dirs_to_check.append(filter_dir + QStringLiteral("/")); // added "/" so all entries have base + 1 length
        }

        // run through the directories
        for (const QString& subdir : qAsConst(dirs_to_check)) {
            QDirIterator subdir_it(subdir, entry_filters, entry_flags);
            while (subdir_it.hasNext()) {
                subdir_it.next();
                QFileInfo fileinfo = subdir_it.fileInfo();
                const QString relative_path = fileinfo.filePath().mid(filter_dir.length() + 1);

                const bool exclude = filter.exclude.extensions.contains(fileinfo.suffix())
                    || filter.exclude.files.contains(relative_path)
                    || (!filter.exclude.regex.isEmpty() && exclude_regex.match(fileinfo.filePath()).hasMatch());
                if (exclude)
                    continue;

                const bool include = filter.include.extensions.contains(fileinfo.suffix())
                    || filter.include.files.contains(relative_path)
                    || (!filter.include.regex.isEmpty() && include_regex.match(fileinfo.filePath()).hasMatch());
                if (!include)
                    continue;

                const QString game_key = fileinfo.canonicalFilePath();
                if (!games.count(game_key)) {
                    modeldata::Game game(std::move(fileinfo));
                    game.launch_cmd = collections.at(filter.parent_collection).launch_cmd;
                    games.emplace(game_key, std::move(game));
                }
                collection_childs[filter.parent_collection].emplace_back(game_key);
            }
        }
    }
}

} // namespace


namespace providers {
namespace pegasus {

PegasusCollections::PegasusCollections()
    : m_coll_attribs {
        { QStringLiteral("shortname"), CollAttrib::SHORT_NAME },
        { QStringLiteral("launch"), CollAttrib::LAUNCH_CMD },
        { QStringLiteral("command"), CollAttrib::LAUNCH_CMD },
        { QStringLiteral("workdir"), CollAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("working-directory"), CollAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("cwd"), CollAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("directory"), CollAttrib::DIRECTORIES },
        { QStringLiteral("directories"), CollAttrib::DIRECTORIES },
        { QStringLiteral("extension"), CollAttrib::EXTENSIONS },
        { QStringLiteral("extensions"), CollAttrib::EXTENSIONS },
        { QStringLiteral("file"), CollAttrib::FILES },
        { QStringLiteral("files"), CollAttrib::FILES },
        { QStringLiteral("regex"), CollAttrib::REGEX },
        { QStringLiteral("ignore-extension"), CollAttrib::EXTENSIONS },
        { QStringLiteral("ignore-extensions"), CollAttrib::EXTENSIONS },
        { QStringLiteral("ignore-file"), CollAttrib::FILES },
        { QStringLiteral("ignore-files"), CollAttrib::FILES },
        { QStringLiteral("ignore-regex"), CollAttrib::REGEX },
        { QStringLiteral("summary"), CollAttrib::SHORT_DESC },
        { QStringLiteral("description"), CollAttrib::LONG_DESC },
    }
    , m_game_attribs {
        { QStringLiteral("file"), GameAttrib::FILES },
        { QStringLiteral("files"), GameAttrib::FILES },
        { QStringLiteral("launch"), GameAttrib::LAUNCH_CMD },
        { QStringLiteral("command"), GameAttrib::LAUNCH_CMD },
        { QStringLiteral("workdir"), GameAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("working-directory"), GameAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("cwd"), GameAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("developer"), GameAttrib::DEVELOPERS },
        { QStringLiteral("developers"), GameAttrib::DEVELOPERS },
        { QStringLiteral("publisher"), GameAttrib::PUBLISHERS },
        { QStringLiteral("publishers"), GameAttrib::PUBLISHERS },
        { QStringLiteral("genre"), GameAttrib::GENRES },
        { QStringLiteral("genres"), GameAttrib::GENRES },
        { QStringLiteral("players"), GameAttrib::PLAYER_COUNT },
        { QStringLiteral("summary"), GameAttrib::SHORT_DESC },
        { QStringLiteral("description"), GameAttrib::LONG_DESC },
        { QStringLiteral("release"), GameAttrib::RELEASE },
        { QStringLiteral("rating"), GameAttrib::RATING },
    }
{
}

void PegasusCollections::find_in_dirs(const std::vector<QString>& dir_list,
                                      HashMap<QString, modeldata::Game>& games,
                                      HashMap<QString, modeldata::Collection>& collections,
                                      HashMap<QString, std::vector<QString>>& collection_childs,
                                      const std::function<void(int)>& update_gamecount_maybe) const
{
    std::vector<GameFilter> all_filters;

    for (const QString& dir_path : dir_list) {
        auto filters = read_collections_file(m_key_types, dir_path, collections);
        all_filters.reserve(all_filters.size() + filters.size());
        all_filters.insert(all_filters.end(),
                           std::make_move_iterator(filters.begin()),
                           std::make_move_iterator(filters.end()));
    }
    for (const GameFilter& filter : all_filters)
        process_filter(filter, games, collections, collection_childs);

    update_gamecount_maybe(static_cast<int>(games.size()));
}

} // namespace pegasus
} // namespace providers
