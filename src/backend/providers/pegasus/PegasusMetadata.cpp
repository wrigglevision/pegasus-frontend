// Pegasus Frontend
// Copyright (C) 2017-2019  Mátyás Mustoha
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


#include "PegasusMetadata.h"

#include "ConfigFile.h"
#include "LocaleUtils.h"
#include "Paths.h"
#include "PegasusAssets.h"
#include "PegasusParser.h"
#include "PegasusUtils.h"
#include "modeldata/gaming/CollectionData.h"
#include "modeldata/gaming/GameData.h"
#include "utils/ParserTools.h"
#include "utils/PathCheck.h"

#include <QDebug>
#include <QDirIterator>
#include <QRegularExpression>
#include <QStringBuilder>


namespace {
using namespace providers::pegasus::parser;
using namespace providers::pegasus::utils;

static constexpr auto MSG_PREFIX = "Collections:";


const QString& first_line_of(const config::Entry& entry)
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
    Q_ASSERT(!dir_path.isEmpty());

    // TODO: move metadata first after some transition period
    const QString possible_paths[] {
        dir_path + QStringLiteral("/collections.pegasus.txt"),
        dir_path + QStringLiteral("/metadata.pegasus.txt"),
        dir_path + QStringLiteral("/collections.txt"),
        dir_path + QStringLiteral("/metadata.txt"),
    };

    for (const QString& path : possible_paths) {
        if (QFileInfo::exists(path)) {
            qInfo().noquote() << MSG_PREFIX
                << tr_log("found `%1`").arg(path);
            return path;
        }
    }

    qWarning().noquote() << MSG_PREFIX
        << tr_log("No metadata file found in `%1`, directory ignored").arg(dir_path);
    return QString();
}

void parse_collection_entry(ParserContext& ctx, const config::Entry& entry)
{
    Q_ASSERT(ctx.cur_coll);
    Q_ASSERT(ctx.cur_filter);
    Q_ASSERT(!ctx.cur_game);

    if (!ctx.helpers.coll_attribs.count(entry.key)) {
        ctx.print_error(entry.line, tr_log("unrecognized collection property `%1`, ignored").arg(entry.key));
        return;
    }

    FileFilterGroup& filter_group = entry.key.startsWith(QLatin1String("ignore-"))
        ? ctx.cur_filter->exclude
        : ctx.cur_filter->include;

    switch (ctx.helpers.coll_attribs.at(entry.key)) {
        case CollAttrib::SHORT_NAME:
            ctx.cur_coll->setShortName(first_line_of(entry));
            break;
        case CollAttrib::LAUNCH_CMD:
            ctx.cur_coll->launch_cmd = config::mergeLines(entry.values);
            break;
        case CollAttrib::DIRECTORIES:
            for (const QString& value : entry.values) {
                QFileInfo finfo(value);
                if (finfo.isRelative())
                    finfo.setFile(ctx.dir_path % '/' % value);

                ctx.cur_filter->directories.append(finfo.canonicalFilePath());
            }
            break;
        case CollAttrib::EXTENSIONS:
            filter_group.extensions.append(tokenize_by_comma(first_line_of(entry).toLower()));
            break;
        case CollAttrib::FILES:
            for (const QString& value : entry.values)
                filter_group.files.append(value);
            break;
        case CollAttrib::REGEX:
            filter_group.regex = first_line_of(entry);
            break;
        case CollAttrib::SHORT_DESC:
            ctx.cur_coll->summary = config::mergeLines(entry.values);
            break;
        case CollAttrib::LONG_DESC:
            ctx.cur_coll->description = config::mergeLines(entry.values);
            break;
        case CollAttrib::LAUNCH_WORKDIR:
            ctx.cur_coll->launch_workdir = first_line_of(entry);
            break;
    }
}

void parse_game_entry(ParserContext& ctx, const config::Entry& entry)
{
    // NOTE: ctx.cur_coll may be null (ie. a game entry defined before any collection)
    Q_ASSERT(ctx.cur_game);

    if (!ctx.helpers.game_attribs.count(entry.key)) {
        ctx.print_error(entry.line, tr_log("unrecognized game property `%1`, ignored").arg(entry.key));
        return;
    }

    switch (ctx.helpers.game_attribs.at(entry.key)) {
        case GameAttrib::FILES:
            for (const QString& line : entry.values) {
                QFileInfo finfo(line);
                if (finfo.isRelative())
                    finfo.setFile(ctx.dir_path % '/' % line);

                const QString abs_path = finfo.absoluteFilePath();
                const QString can_path = finfo.canonicalFilePath();

                ctx.cur_game->files.emplace(abs_path, finfo);
                ctx.outvars.sctx.path_to_gameidx[can_path] = ctx.outvars.sctx.games.size() - 1;
            }
            break;
        case GameAttrib::DEVELOPERS:
            for (const QString& line : entry.values)
                ctx.cur_game->developers.append(line);
            break;
        case GameAttrib::PUBLISHERS:
            for (const QString& line : entry.values)
                ctx.cur_game->publishers.append(line);
            break;
        case GameAttrib::GENRES:
            for (const QString& line : entry.values)
                ctx.cur_game->genres.append(line);
            break;
        case GameAttrib::PLAYER_COUNT:
            {
                const auto rx_match = ctx.helpers.rx_count_range.match(first_line_of(entry));
                if (rx_match.hasMatch()) {
                    const int a = rx_match.capturedRef(1).toInt();
                    const int b = rx_match.capturedRef(3).toInt();
                    ctx.cur_game->player_count = std::max({1, a, b});
                }
            }
            break;
        case GameAttrib::SHORT_DESC:
            ctx.cur_game->summary = config::mergeLines(entry.values);
            break;
        case GameAttrib::LONG_DESC:
            ctx.cur_game->description = config::mergeLines(entry.values);
            break;
        case GameAttrib::RELEASE:
            {
                const auto rx_match = ctx.helpers.rx_date.match(first_line_of(entry));
                if (!rx_match.hasMatch()) {
                    ctx.print_error(entry.line, tr_log("incorrect date format, should be YYYY, YYYY-MM or YYYY-MM-DD"));
                    return;
                }

                const int y = qMax(1, rx_match.captured(1).toInt());
                const int m = qBound(1, rx_match.captured(3).toInt(), 12);
                const int d = qBound(1, rx_match.captured(5).toInt(), 31);
                ctx.cur_game->release_date = QDate(y, m, d);
            }
            break;
        case GameAttrib::RATING:
            {
                const QString& line = first_line_of(entry);

                const auto rx_match_a = ctx.helpers.rx_percent.match(line);
                if (rx_match_a.hasMatch()) {
                    ctx.cur_game->rating = qBound(0.f, line.leftRef(line.length() - 1).toFloat() / 100.f, 1.f);
                    return;
                }
                const auto rx_match_b = ctx.helpers.rx_float.match(line);
                if (rx_match_b.hasMatch()) {
                    ctx.cur_game->rating = qBound(0.f, line.toFloat() / 100.f, 1.f);
                    return;
                }

                ctx.print_error(entry.line, tr_log("failed to parse rating value"));
            }
            break;
        case GameAttrib::LAUNCH_CMD:
            ctx.cur_game->launch_cmd = first_line_of(entry);
            break;
        case GameAttrib::LAUNCH_WORKDIR:
            ctx.cur_game->launch_workdir = first_line_of(entry);
            break;
    }
}

// Returns true if the entry key matches asset regex.
// The actual asset type check may still fail however.
bool parse_asset_entry_maybe(ParserContext& ctx, const config::Entry& entry)
{
    Q_ASSERT(ctx.cur_coll || ctx.cur_game);

    const auto rx_match = ctx.helpers.rx_asset_key.match(entry.key);
    if (!rx_match.hasMatch())
        return false;

    const QString asset_key = rx_match.captured(1);
    const AssetType asset_type = pegasus_assets::str_to_type(asset_key);
    if (asset_type == AssetType::UNKNOWN) {
        ctx.print_error(entry.line, tr_log("unknown asset type '%1', entry ignored").arg(asset_key));
        return true;
    }

    modeldata::GameAssets& assets = ctx.cur_game
        ? ctx.cur_game->assets
        : ctx.cur_coll->default_assets;
    assets.addUrlMaybe(asset_type, assetline_to_url(entry.values.first(), ctx.dir_path));
    return true;
}

void parse_entry(ParserContext& ctx, const config::Entry& entry)
{
    //qDebug() << "parsing" << entry.key << entry.values;

    // TODO: set cur_* by the return value of emplace
    if (entry.key == QLatin1String("collection")) {
        const QString& name = first_line_of(entry);

        if (!ctx.outvars.sctx.collections.count(name))
            ctx.outvars.sctx.collections.emplace(name, modeldata::Collection(name));

        ctx.cur_coll = &ctx.outvars.sctx.collections.at(name);
        ctx.cur_game = nullptr;

        ctx.outvars.filters.emplace_back(name, ctx.dir_path);
        ctx.cur_filter = &ctx.outvars.filters.back();
        return;
    }

    if (entry.key == QLatin1String("game")) {
        ctx.outvars.sctx.games.emplace_back(first_line_of(entry));
        ctx.cur_game = &ctx.outvars.sctx.games.back();
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


    if (ctx.cur_game)
        parse_game_entry(ctx, entry);
    else
        parse_collection_entry(ctx, entry);
}

void tidy_filters(ParserContext& ctx)
{
    for (FileFilter& filter : ctx.outvars.filters) {
        filter.directories.removeDuplicates();
        filter.include.extensions.removeDuplicates();
        filter.include.files.removeDuplicates();
        filter.exclude.extensions.removeDuplicates();
        filter.exclude.files.removeDuplicates();
    }
}

void read_metafile(const QString& metafile_path, OutputVars& output, const Helpers& helpers)
{
    ParserContext ctx(metafile_path, output, helpers);

    const auto on_error = [&](const config::Error& error){
        ctx.print_error(error.line, error.message);
    };
    const auto on_entry = [&](const config::Entry& entry){
        parse_entry(ctx, entry);
    };

    if (!config::readFile(metafile_path, on_entry, on_error)) {
        qWarning().noquote() << MSG_PREFIX
            << tr_log("Failed to read metadata file %1, file ignored").arg(metafile_path);
        return;
    }

    tidy_filters(ctx);
}


// Find all dirs and subdirectories, but ignore 'media'
QVector<QString> filter_find_dirs(const QString& filter_dir)
{
    constexpr auto subdir_filters = QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot;
    constexpr auto subdir_flags = QDirIterator::FollowSymlinks | QDirIterator::Subdirectories;

    QVector<QString> result;

    QDirIterator dirs_it(filter_dir, subdir_filters, subdir_flags);
    while (dirs_it.hasNext())
        result << dirs_it.next();

    result.removeOne(filter_dir + QStringLiteral("/media"));
    result.append(filter_dir + QStringLiteral("/")); // added "/" so all entries have base + 1 length

    return result;
}

void process_filter(const FileFilter& filter, OutputVars& out)
{
    constexpr auto entry_filters = QDir::Files | QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot;
    constexpr auto entry_flags = QDirIterator::FollowSymlinks;

    const QRegularExpression include_regex(filter.include.regex);
    const QRegularExpression exclude_regex(filter.exclude.regex);


    for (const QString& filter_dir : filter.directories)
    {
        //qDebug() << "Running filter for" << filter.collection_name << "in" << filter_dir;
        const QVector<QString> dirs_to_check = filter_find_dirs(filter_dir);
        for (const QString& subdir : dirs_to_check) {
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

                const QString game_path = fileinfo.canonicalFilePath();
                if (!out.sctx.path_to_gameidx.count(game_path)) {
                    // This means there weren't any game entries with matching file entry
                    // in any of the parsed metadata files. There is no existing game data
                    // created yet either.
                    const modeldata::Collection& parent = out.sctx.collections.at(filter.collection_name);
                    modeldata::Game game(fileinfo);
                    game.launch_cmd = parent.launch_cmd;
                    game.launch_workdir = parent.launch_workdir;

                    out.sctx.path_to_gameidx.emplace(game_path, out.sctx.games.size());
                    out.sctx.games.emplace_back(std::move(game));
                }
                const size_t game_idx = out.sctx.path_to_gameidx.at(game_path);
                out.sctx.collection_childs[filter.collection_name].emplace_back(game_idx);
            }
        }
    }
}

} // namespace


namespace providers {
namespace pegasus {

void find_in_dirs(const std::vector<QString>& dir_list, providers::SearchContext& sctx)
{
    const Helpers helpers;
    OutputVars output { sctx, {} };

    // collect collection and game information
    for (const QString& dir_path : dir_list) {
        const QString metafile = find_metafile_in(dir_path);
        if (metafile.isEmpty())
            continue;

        read_metafile(metafile, output, helpers);
    }

    // find the actually existing files and assign them to the data
    for (const FileFilter& filter : output.filters)
        process_filter(filter, output);

    // remove empty games
    auto it = std::remove_if(sctx.games.begin(), sctx.games.end(),
        [](const modeldata::Game& game) { return game.launch_cmd.isEmpty() && game.files.empty(); });
    sctx.games.erase(it, sctx.games.end());
}

} // namespace pegasus
} // namespace providers
