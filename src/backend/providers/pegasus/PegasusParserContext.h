#pragma once

#include "modeldata/gaming/CollectionData.h"
#include "modeldata/gaming/GameData.h"
#include "utils/HashMap.h"
#include "utils/MoveOnly.h"

#include <QRegularExpression>
#include <QString>
#include <vector>


namespace providers {
namespace pegasus {
namespace parser {

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
    // TODO: COLLECTION,
};
// TODO: in the future
/*enum class GameFileAttrib : unsigned char {
    TITLE,
    SHORT_DESC,
    LONG_DESC,
    LAUNCH_CMD,
    LAUNCH_WORKDIR,
};*/


struct FileFilterGroup {
    QStringList extensions;
    QStringList files;
    QString regex;

    FileFilterGroup();
    MOVE_ONLY(FileFilterGroup)
};
struct FileFilter {
    // NOTE: collections can have different filtering parameters in different directories
    QString collection_name;
    QStringList directories;
    FileFilterGroup include;
    FileFilterGroup exclude;

    FileFilter(QString collection, QString base_dir);
    MOVE_ONLY(FileFilter)
};

struct OutputVars {
    HashMap<QString, modeldata::Collection>& collections;
    HashMap<QString, std::vector<size_t>>& collection_childs;
    std::vector<modeldata::Game>& games;

    HashMap<QString, size_t> path_to_gameidx;
    std::vector<FileFilter> filters;

    MOVE_ONLY(OutputVars)
};

struct Helpers {
    const HashMap<QString, CollAttrib> coll_attribs;
    const HashMap<QString, GameAttrib> game_attribs;
    //const HashMap<QString, GameFileAttrib> gamefile_attribs;
    const QRegularExpression rx_asset_key;
    const QRegularExpression rx_count_range;
    const QRegularExpression rx_percent;
    const QRegularExpression rx_float;
    const QRegularExpression rx_date;

    Helpers();
    MOVE_ONLY(Helpers)
};

struct ParserContext {
    const QString metafile_path;
    const QString dir_path;

    OutputVars& outvars;
    const Helpers& helpers;

    modeldata::Collection* cur_coll;
    // NOTE: while these would be highly unsafe normally, we can use the fact
    // that no games/filters are added during the time their pointer is used
    FileFilter* cur_filter;
    modeldata::Game* cur_game;


    ParserContext(QString metafile_path, OutputVars&, const Helpers&);
    MOVE_ONLY(ParserContext)

    void print_error(const int lineno, const QString msg) const;
};

} // namespace parser
} // namespace pegasus
} // namespace providers
