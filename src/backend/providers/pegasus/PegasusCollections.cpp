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

/*struct ParserContext {
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
};*/



std::vector<GameFilter> read_collections_file(const HashMap<QString, CollAttrib>& key_types,
                                              const QString& dir_path,
                                              HashMap<QString, modeldata::Collection>& collections)
{
    // reminder: sections are collection names
    // including keys: extensions, files, regex
    // excluding keys: ignore-extensions, ignore-files, ignore-regex
    // optional: name, launch, directories

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



const QString& entry_first_line(const config::Entry& entry)
{
    Q_ASSERT(!entry.key.isEmpty());
    Q_ASSERT(!entry.values.isEmpty());

    if (entry.values.count() > 1) {
        qWarning().noquote() << MSG_PREFIX
            << tr_log("Expected single line value for `%1`but got more. The rest of the lines will be ignored.")
               .arg(entry.key);
    }

    return entry.values.first();
}

QString find_metafile_in(const QString& dir_path)
{
    const QString possible_paths[] {
        dir_path + QStringLiteral("/metadata.pegasus.txt"),
        dir_path + QStringLiteral("/collections.pegasus.txt"),
        dir_path + QStringLiteral("/metadata.txt"),
        dir_path + QStringLiteral("/collections.txt"),
    };

    for (const QString& path : possible_paths) {
        if (QFileInfo::exists(path))
            return path;
    }

    return QString();
}

struct FileFilterGroup {
    QStringList extensions;
    QStringList files;
    QString regex;
};
struct FileFilter {
    QString collection_name;
    QVector<QString> directories;
    FileFilterGroup include;
    FileFilterGroup exclude;

    FileFilter(QString collection, QString base_dir)
        : collection_name(std::move(collection))
        , directories({std::move(base_dir)})
    {}

    MOVE_ONLY(FileFilter)
};

struct DataMaps {
    HashMap<QString, modeldata::Game>& games;
    HashMap<QString, modeldata::Collection>& collections;
    HashMap<QString, std::vector<QString>>& collection_childs;
};

struct Helpers {
    const HashMap<QString, CollAttrib> coll_attribs;
    const HashMap<QString, GameAttrib> game_attribs;
    const QRegularExpression rx_asset_key;
    const QRegularExpression rx_count_range;
    const QRegularExpression rx_percent;
    const QRegularExpression rx_float;
    const QRegularExpression rx_date;

    Helpers()
        : coll_attribs {
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
        , game_attribs {
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
        , rx_asset_key(QStringLiteral(R"(^assets?\.(.+)$)"))
        , rx_count_range(QStringLiteral("^(\\d+)(-(\\d+))?$"))
        , rx_percent(QStringLiteral("^\\d+%$"))
        , rx_float(QStringLiteral("^\\d(\\.\\d+)?$"))
        , rx_date(QStringLiteral("^(\\d{4})(-(\\d{1,2}))?(-(\\d{1,2}))?$"))
    {}
};

struct ParserContext {
    const QString cur_dir;
    const QString cur_metafile_path;

    DataMaps& maps;
    Helpers& helpers;

    HashMap<QString, FileFilter> filters;
    modeldata::Collection* cur_coll;
    modeldata::Game* cur_game;
    FileFilter* cur_filter;
    bool parsing_game;

    ParserContext(QString base_dir, QString metafile_path, DataMaps& maps, Helpers& helpers)
        : cur_dir(std::move(base_dir))
        , cur_metafile_path(std::move(metafile_path))
        , maps(maps)
        , helpers(helpers)
        , cur_coll(nullptr)
        , cur_filter(nullptr)
        , parsing_game(false)
    {}

    void print_error(const int lineno, const QString msg){
        qWarning().noquote() << MSG_PREFIX
            << tr_log("`%1`, line %2: %3").arg(cur_metafile_path, QString::number(lineno), msg);
    }
};

void parse_collection_entry(ParserContext& ctx, const config::Entry& entry)
{
    Q_ASSERT(ctx.cur_coll);
    Q_ASSERT(ctx.cur_filter);
    Q_ASSERT(!ctx.parsing_game);
    Q_ASSERT(!ctx.cur_game);

    if (!ctx.helpers.coll_attribs.count(entry.key)) {
        ctx.print_error(entry.line, tr_log("unrecognized collection property `%3`, ignored").arg(entry.key));
        return;
    }

    FileFilterGroup& filter_group = entry.key.startsWith(QLatin1String("ignore-"))
        ? ctx.cur_filter->exclude
        : ctx.cur_filter->include;

    switch (ctx.helpers.coll_attribs.at(entry.key)) {
        case CollAttrib::SHORT_NAME:
            ctx.cur_coll->setShortName(entry_first_line(entry));
            break;
        case CollAttrib::LAUNCH_CMD:
            ctx.cur_coll->launch_cmd = config::mergeLines(entry.values);
            break;
        case CollAttrib::DIRECTORIES:
            for (const QString& value : entry.values) {
                QFileInfo finfo(value);
                if (finfo.isRelative())
                    finfo.setFile(ctx.cur_dir % '/' % value);

                ctx.cur_filter->directories.append(finfo.canonicalFilePath());
            }
            break;
        case CollAttrib::EXTENSIONS:
            filter_group.extensions.append(providers::pegasus::tokenize(entry_first_line(entry).toLower()));
            break;
        case CollAttrib::FILES:
            for (const QString& value : entry.values)
                filter_group.files.append(value);
            break;
        case CollAttrib::REGEX:
            filter_group.regex = entry_first_line(entry);
            break;
        case CollAttrib::SHORT_DESC:
            ctx.cur_coll->summary = config::mergeLines(entry.values);
            break;
        case CollAttrib::LONG_DESC:
            ctx.cur_coll->description = config::mergeLines(entry.values);
            break;
        case CollAttrib::LAUNCH_WORKDIR:
            ctx.cur_coll->launch_workdir = entry_first_line(entry);
            break;
    }
}

QVector<QString> tokenize_game_file_line(const QString& line)
{
    Q_ASSERT(line.trimmed() == line);

    QVector<QString> parts;
    QString current;

    bool in_quote = false;
    QChar quote_kar = '"';


    for (int i = 0; i < line.length(); i++) {
        const QChar kar = line.at(i);

        // escape
        if (kar == QLatin1Char('\\')) {
            if (i + 1 < line.length())
                current += line.at(++i);

            continue;
        }

        const bool is_quote_char = (kar == QLatin1Char('\'') || kar == QLatin1Char('"'));
        if (is_quote_char) {
            // try to start or end the quote
            if (!in_quote) {
                in_quote = true;
                quote_kar = kar;
            }
            else if (kar == quote_kar) {
                in_quote = false;
            }
            // otherwise go on
        }

        if (kar.isSpace()) {
            if (!in_quote) {
                if (!current.isEmpty()) {
                    parts.append(current);
                    current.clear();
                }
            }
        }

        if (!in_quote && kar.isSpace()) {
        }


    }
}

void parse_game_entry_file_line(ParserContext& ctx, const QString& line)
{
    const auto parts = line.splitRef(QChar)
    QFileInfo finfo(value);
    if (finfo.isRelative())
        finfo.setFile(ctx.cur_dir % '/' % value);

}

void parse_game_entry(ParserContext& ctx, const config::Entry& entry)
{
    Q_ASSERT(ctx.cur_coll);
    Q_ASSERT(ctx.parsing_game);
    Q_ASSERT(ctx.cur_game);

    if (!ctx.helpers.game_attribs.count(entry.key)) {
        ctx.print_error(entry.line, tr_log("unrecognized game property `%3`, ignored").arg(entry.key));
        return;
    }

    switch (ctx.helpers.game_attribs.at(entry.key)) {
        case GameAttrib::FILES:
            for (const QString& value : entry.values) {

                ctx.cur_game->files.append(value);
            }
            break;
        case MetaAttribType::DEVELOPER:
            curr_game->developers.append(config::mergeLines(entry.values));
            break;
        /*case MetaAttribType::PUBLISHER:
            curr_game->publishers.append(val);
            break;
        case MetaAttribType::GENRE:
            curr_game->genres.append(tokenize(val));
            break;
        case MetaAttribType::PLAYER_COUNT:
            {
                const auto rx_match = m_player_regex.match(val);
                if (rx_match.hasMatch()) {
                    const int a = rx_match.capturedRef(1).toInt();
                    const int b = rx_match.capturedRef(3).toInt();
                    curr_game->player_count = qMax(1, qMax(a, b));
                }
            }
            break;
        case MetaAttribType::SHORT_DESC:
            curr_game->summary = val;
            break;
        case MetaAttribType::LONG_DESC:
            curr_game->description = val;
            break;
        case MetaAttribType::RELEASE:
            {
                const auto rx_match = m_release_regex.match(val);
                if (!rx_match.hasMatch()) {
                    on_error(entry.line, tr_log("incorrect date format, should be YYYY(-MM(-DD))"));
                    return;
                }

                const int y = qMax(1, rx_match.captured(1).toInt());
                const int m = qBound(1, rx_match.captured(3).toInt(), 12);
                const int d = qBound(1, rx_match.captured(5).toInt(), 31);
                curr_game->release_date = QDate(y, m, d);
            }
            break;
        case MetaAttribType::RATING:
            {
                const auto rx_match_a = m_rating_percent_regex.match(val);
                if (rx_match_a.hasMatch()) {
                    curr_game->rating = qBound(0.f, val.leftRef(val.length() - 1).toFloat() / 100.f, 1.f);
                    return;
                }
                const auto rx_match_b = m_rating_float_regex.match(val);
                if (rx_match_b.hasMatch()) {
                    curr_game->rating = qBound(0.f, val.toFloat() / 100.f, 1.f);
                    return;
                }
                on_error(lineno, tr_log("failed to parse rating value"));
            }
            break;
        case MetaAttribType::LAUNCH_CMD:
            curr_game->launch_cmd = val;
            break;
        case MetaAttribType::LAUNCH_WORKDIR:
            curr_game->launch_workdir = val;
            break;*/
    }
}

// Returns true if the entry key matches asset regex.
// The actual asset type check may still fail however.
bool parse_asset_entry_maybe(ParserContext& ctx, const config::Entry& entry)
{
    Q_ASSERT(ctx.cur_coll);
    Q_ASSERT(ctx.parsing_game && ctx.cur_game);

    const auto rx_match = ctx.helpers.rx_asset_key.match(entry.key);
    if (!rx_match.hasMatch())
        return false;

    const QString asset_key = rx_match.captured(1);
    const AssetType asset_type = pegasus_assets::str_to_type(asset_key);
    if (asset_type == AssetType::UNKNOWN) {
        ctx.print_error(entry.line, tr_log("unknown asset type '%1', entry ignored").arg(asset_key));
        return true;
    }

    modeldata::GameAssets& assets = ctx.parsing_game
        ? ctx.cur_coll->default_assets
        : ctx.cur_game->assets;
    providers::pegasus::add_asset(assets, asset_type, entry.values.first(), ctx.cur_dir);
    return true;
}

void parse_entry(ParserContext& ctx, const config::Entry& entry)
{
    // TODO: set cur_* by the return value of emplace
    if (entry.key == QLatin1String("collection")) {
        const QString& name = entry_first_line(entry);

        if (!ctx.maps.collections.count(name))
            ctx.maps.collections.emplace(name, modeldata::Collection(name));
        if (!ctx.filters.count(name))
            ctx.filters.emplace(name, ctx.cur_dir);

        ctx.cur_game = nullptr;
        ctx.cur_coll = &ctx.maps.collections.at(name);
        ctx.cur_filter = &ctx.filters.at(name);
        ctx.parsing_game = false;
        return;
    }

    if (entry.key == QLatin1String("game")) {
        const QString name = entry_first_line(entry);

        if (!ctx.maps.games.count(name))
            ctx.maps.games.emplace(name, modeldata::Game(name));

        ctx.cur_game = &ctx.maps.games.at(name);
        ctx.parsing_game = true;
        return;
    }


    if (!ctx.cur_coll && !ctx.cur_game) {
        ctx.print_error(entry.line, tr_log("no `collection` or `game` defined yet, entry ignored"));
        return;
    }

    if (entry.key.startsWith(QLatin1String("x-")))
        return;

    if (parse_asset_entry_maybe(ctx, entry))
        return;


    if (ctx.parsing_game)
        parse_game_entry(ctx, entry);
    else
        parse_collection_entry(ctx, entry);
}

MetafileData read_metafile(const QString& metafile_path,
                           DataMaps& maps)
{
    MetafileData result;

    const auto on_error = [&](const config::Error& error){
        qWarning().noquote() << MSG_PREFIX
            << tr_log("`%1`, line %2: %3").arg(metafile_path, QString::number(error.line), error.message);
    };
    const auto on_entry = [&](const config::Entry& entry){
        parse_entry(maps, result, entry);
    };

    if (!config::readFile(metafile_path, on_entry, on_error)) {
        qWarning().noquote() << MSG_PREFIX
            << tr_log("Failed to read metadata file %1, file ignored").arg(metafile_path);
    }
    return result;
}

} // namespace


namespace providers {
namespace pegasus {

PegasusCollections::PegasusCollections()
{
}

void PegasusCollections::find_in_dirs(const std::vector<QString>& dir_list,
                                      HashMap<QString, modeldata::Game>& games,
                                      HashMap<QString, modeldata::Collection>& collections,
                                      HashMap<QString, std::vector<QString>>& collection_childs,
                                      const std::function<void(int)>& update_gamecount_maybe) const
{
    DataMaps maps { games, collections, collection_childs };
    //std::vector<GameFilter> all_filters;

    for (const QString& dir_path : dir_list) {
        const QString metafile = find_metafile_in(dir_path);
        if (metafile.isEmpty()) {
            qWarning().noquote() << MSG_PREFIX
                << tr_log("No metadata file found in %1, directory ignored").arg(dir_path);
            continue;
        }

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
