#include "PegasusParserContext.h"

#include "LocaleUtils.h"

#include <QDebug>


namespace {
static constexpr auto MSG_PREFIX = "Collections:";
} // namespace


namespace providers {
namespace pegasus {
namespace parser {

FileFilterGroup::FileFilterGroup() = default;

FileFilter::FileFilter(QString collection, QString base_dir)
    : collection_name(std::move(collection))
    , directories({std::move(base_dir)})
{
    Q_ASSERT(!directories.front().isEmpty());
}


Helpers::Helpers()
    : coll_attribs {
        { QStringLiteral("shortname"), CollAttrib::SHORT_NAME },
        { QStringLiteral("launch"), CollAttrib::LAUNCH_CMD },
        { QStringLiteral("command"), CollAttrib::LAUNCH_CMD },
        { QStringLiteral("workdir"), CollAttrib::LAUNCH_WORKDIR },
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
    /*, gamefile_attribs {
        { QStringLiteral("name"), GameFileAttrib::TITLE },
        { QStringLiteral("title"), GameFileAttrib::TITLE },
        { QStringLiteral("launch"), GameFileAttrib::LAUNCH_CMD },
        { QStringLiteral("command"), GameFileAttrib::LAUNCH_CMD },
        { QStringLiteral("workdir"), GameFileAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("cwd"), GameFileAttrib::LAUNCH_WORKDIR },
        { QStringLiteral("summary"), GameFileAttrib::SHORT_DESC },
        { QStringLiteral("description"), GameFileAttrib::LONG_DESC },
    }*/
    , rx_asset_key(QStringLiteral(R"(^assets?\.(.+)$)"))
    , rx_count_range(QStringLiteral("^(\\d+)(-(\\d+))?$"))
    , rx_percent(QStringLiteral("^\\d+%$"))
    , rx_float(QStringLiteral("^\\d(\\.\\d+)?$"))
    , rx_date(QStringLiteral("^(\\d{4})(-(\\d{1,2}))?(-(\\d{1,2}))?$"))
{}


ParserContext::ParserContext(QString file_path, OutputVars& outvars, const Helpers& helpers)
    : metafile_path(std::move(file_path))
    , dir_path(QFileInfo(metafile_path).path())
    , outvars(outvars)
    , helpers(helpers)
    , cur_coll(nullptr)
    , cur_filter(nullptr)
    , cur_game(nullptr)
{
    Q_ASSERT(!metafile_path.isEmpty());
    Q_ASSERT(!dir_path.isEmpty());
}

void ParserContext::print_error(const int lineno, const QString msg) const {
    qWarning().noquote() << MSG_PREFIX
        << tr_log("`%1`, line %2: %3").arg(metafile_path, QString::number(lineno), msg);
}

} // namespace parser
} // namespace pegasus
} // namespace providers
