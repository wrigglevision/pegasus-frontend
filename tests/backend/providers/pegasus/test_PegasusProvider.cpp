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


#include <QtTest/QtTest>

#include "providers/pegasus/PegasusProvider.h"
#include "modeldata/gaming/CollectionData.h"
#include "modeldata/gaming/GameData.h"
#include "utils/HashMap.h"

#include <QString>


class test_PegasusProvider : public QObject {
    Q_OBJECT

private slots:
    void empty();
    void simple();
    void with_meta();
    /*void asset_search();
    void custom_assets();
    void custom_directories();*/
};

void test_PegasusProvider::empty()
{
    providers::SearchContext ctx;

    QTest::ignoreMessage(QtWarningMsg, "Collections: No metadata file found in `:/empty`, directory ignored");
    providers::pegasus::PegasusProvider provider({QStringLiteral(":/empty")});
    provider.findLists(ctx);

    QVERIFY(ctx.games.empty());
    QVERIFY(ctx.collections.empty());
    QVERIFY(ctx.collection_childs.empty());
}

void test_PegasusProvider::simple()
{
    providers::SearchContext ctx;

    QTest::ignoreMessage(QtInfoMsg, "Collections: found `:/simple/metadata.pegasus.txt`");
    providers::pegasus::PegasusProvider provider({QStringLiteral(":/simple")});
    provider.findLists(ctx);

    // finds the correct collections
    QCOMPARE(static_cast<int>(ctx.collections.size()), 3);
    QVERIFY(ctx.collections.count(QStringLiteral("My Games")));
    QVERIFY(ctx.collections.count(QStringLiteral("Favorite games")));
    QVERIFY(ctx.collections.count(QStringLiteral("Multi-game ROMs")));

    // finds the correct amount of games
    QCOMPARE(static_cast<int>(ctx.games.size()), 8);
    QVERIFY(ctx.collection_childs.at(QStringLiteral("My Games")).size() == 8);
    QVERIFY(ctx.collection_childs.at(QStringLiteral("Favorite games")).size() == 3);
    QVERIFY(ctx.collection_childs.at(QStringLiteral("Multi-game ROMs")).size() == 1);

    // finds the correct files for the collections
    const QStringList mygames_paths {
        { ":/simple/mygame1.ext" },
        { ":/simple/mygame2.ext" },
        { ":/simple/mygame3.ext" },
        { ":/simple/favgame1.ext" },
        { ":/simple/favgame2.ext" },
        { ":/simple/game with spaces.ext" },
        { ":/simple/9999-in-1.ext" },
        { ":/simple/subdir/game_in_subdir.ext" },
    };
    const QStringList faves_paths {
        { ":/simple/favgame1.ext" },
        { ":/simple/favgame2.ext" },
        { ":/simple/game with spaces.ext" },
    };
    const QStringList multi_paths {
        { ":/simple/9999-in-1.ext" },
    };
    const HashMap<QString, const QStringList* const> coll_files_map {
        { QStringLiteral("My Games"), &mygames_paths },
        { QStringLiteral("Favorite games"), &faves_paths },
        { QStringLiteral("Multi-game ROMs"), &multi_paths },
    };

    for (const auto& entry : coll_files_map) {
        const QString& coll_name = entry.first;
        const QStringList& coll_files = *entry.second;

        std::vector<size_t> expected_indices;

        for (const QString& path : coll_files) {
            QVERIFY(ctx.path_to_gameidx.count(path));
            const size_t game_idx = ctx.path_to_gameidx.at(path);

            // NOTE: paths are not canonical in game.files,
            // just that in these tests they happen to be the same
            QVERIFY(game_idx < ctx.games.size());
            QVERIFY(ctx.games.at(game_idx).files.count(path));

            expected_indices.emplace_back(game_idx);
        }

        std::vector<size_t> actual_indices = ctx.collection_childs.at(coll_name);

        std::sort(actual_indices.begin(), actual_indices.end());
        std::sort(expected_indices.begin(), expected_indices.end());
        QCOMPARE(actual_indices, expected_indices);
    }
}

void test_PegasusProvider::with_meta()
{
    providers::SearchContext ctx;

    QTest::ignoreMessage(QtInfoMsg, "Collections: found `:/with_meta/metadata.pegasus.txt`");
    providers::pegasus::PegasusProvider provider({QStringLiteral(":/with_meta")});
    provider.findLists(ctx);

    const QString collection_name(QStringLiteral("mygames"));
    QCOMPARE(static_cast<int>(ctx.collections.size()), 1);
    QCOMPARE(static_cast<int>(ctx.collections.count(collection_name)), 1);
    QCOMPARE(ctx.collections.at(collection_name).summary, QStringLiteral("this is the summary"));
    QCOMPARE(ctx.collections.at(collection_name).description, QStringLiteral("this is the description"));
    QCOMPARE(static_cast<int>(ctx.games.size()), 5);

    // Game before the first collection entry
    {
        const QString file_path = QStringLiteral(":/with_meta/pre.ext");
        QVERIFY(ctx.path_to_gameidx.count(file_path));
        const size_t game_idx = ctx.path_to_gameidx.at(file_path);
        QVERIFY(game_idx < ctx.games.size());
    }

    // Basic
    {
        const QString file_path = QStringLiteral(":/with_meta/basic.ext");
        QVERIFY(ctx.path_to_gameidx.count(file_path));
        const size_t game_idx = ctx.path_to_gameidx.at(file_path);
        QVERIFY(game_idx < ctx.games.size());

        const modeldata::Game& game = ctx.games.at(game_idx);
        QCOMPARE(game.title, QStringLiteral("A simple game"));
        QCOMPARE(game.developers, QStringList({"Dev", "Dev with Spaces"}));
        QCOMPARE(game.genres, QStringList({"genre1", "genre2", "genre with spaces"}));
        QCOMPARE(game.player_count, 4);
        QCOMPARE(game.release_date, QDate(1998, 5, 1));
        QCOMPARE(game.description, QStringLiteral("a very long\ndescription"));
        QCOMPARE(game.summary, QString());
        QCOMPARE(static_cast<int>(game.files.size()), 1);
        QCOMPARE(static_cast<int>(game.files.count(file_path)), 1);
    }

    // Subdir
    {
        const QString file_path = QStringLiteral(":/with_meta/subdir/game_in_subdir.ext");
        QVERIFY(ctx.path_to_gameidx.count(file_path));
        const size_t game_idx = ctx.path_to_gameidx.at(file_path);
        QCOMPARE(ctx.games.at(game_idx).title, QStringLiteral("Subdir Game"));
    }

    // Multifile
    {
        const QString file_path_a = QStringLiteral(":/with_meta/multi.a.ext");
        const QString file_path_b = QStringLiteral(":/with_meta/multi.b.ext");
        QVERIFY(ctx.path_to_gameidx.count(file_path_a));
        QVERIFY(ctx.path_to_gameidx.count(file_path_b));
        const size_t game_idx_a = ctx.path_to_gameidx.at(file_path_a);
        const size_t game_idx_b = ctx.path_to_gameidx.at(file_path_b);
        QCOMPARE(game_idx_a, game_idx_b);
        QVERIFY(game_idx_a < ctx.games.size());

        const modeldata::Game& game = ctx.games.at(game_idx_a);
        QCOMPARE(game.title, QStringLiteral("Multifile Game"));
        QCOMPARE(static_cast<int>(game.files.size()), 2);
        QCOMPARE(static_cast<int>(game.files.count(file_path_a)), 1);
        QCOMPARE(static_cast<int>(game.files.count(file_path_b)), 1);
    }

    // Virtual
    {
        const auto it = std::find_if(ctx.games.cbegin(), ctx.games.cend(),
            [](const modeldata::Game& game) { return game.title == QStringLiteral("Virtual Game"); });
        QVERIFY(it != ctx.games.cend());

        const modeldata::Game& game = *it;
        QCOMPARE(static_cast<int>(game.files.size()), 0);
        QCOMPARE(game.launch_cmd, QStringLiteral("runme.exe param1 param2"));
    }
}

/*void test_PegasusProvider::asset_search()
{
    HashMap<QString, modeldata::Game> games;
    HashMap<QString, modeldata::Collection> collections;
    HashMap<QString, std::vector<QString>> collection_childs;

    providers::pegasus::PegasusProvider provider({QStringLiteral(":/asset_search")});

    QTest::ignoreMessage(QtInfoMsg, "Collections: found `:/asset_search/collections.txt`");
    provider.findLists(games, collections, collection_childs);
    provider.findStaticData(games, collections, collection_childs);

    const QString collection_name(QStringLiteral("mygames"));
    QVERIFY(collections.size() == 1);
    QVERIFY(collections.count(collection_name) == 1);
    QVERIFY(games.size() == 4);

    auto game_it = games.find(QStringLiteral(":/asset_search/mygame1.ext"));
    QVERIFY(game_it != games.cend());
    QCOMPARE(game_it->second.assets.single(AssetType::BOX_FRONT),
             QStringLiteral("file::/asset_search/media/mygame1/box_front.png"));
    QCOMPARE(game_it->second.assets.multi(AssetType::VIDEOS),
             { QStringLiteral("file::/asset_search/media/mygame1/video.mp4") });

    game_it = games.find(QStringLiteral(":/asset_search/mygame3.ext"));
    QVERIFY(game_it != games.cend());
    QCOMPARE(game_it->second.assets.multi(AssetType::SCREENSHOTS),
             { QStringLiteral("file::/asset_search/media/mygame3/screenshot.jpg") });
    QCOMPARE(game_it->second.assets.single(AssetType::MUSIC),
             QStringLiteral("file::/asset_search/media/mygame3/music.mp3"));

    game_it = games.find(QStringLiteral(":/asset_search/subdir/mygame4.ext"));
    QVERIFY(game_it != games.cend());
    QCOMPARE(game_it->second.assets.single(AssetType::BACKGROUND),
             QStringLiteral("file::/asset_search/media/subdir/mygame4/background.png"));
}

void test_PegasusProvider::custom_assets()
{
    HashMap<QString, modeldata::Game> games;
    HashMap<QString, modeldata::Collection> collections;
    HashMap<QString, std::vector<QString>> collection_childs;

    QTest::ignoreMessage(QtInfoMsg, "Collections: found `:/custom_assets/collections.txt`");
    QTest::ignoreMessage(QtInfoMsg, "Collections: found `:/custom_assets/metadata.txt`");
    providers::pegasus::PegasusProvider provider({QStringLiteral(":/custom_assets")});
    provider.findLists(games, collections, collection_childs);
    provider.findStaticData(games, collections, collection_childs);

    QVERIFY(collections.size() == 1);
    QVERIFY(games.size() == 1);

    const QString game_key = QStringLiteral(":/custom_assets/mygame1.ext");
    QCOMPARE(games.count(game_key) > 0, true);
    QCOMPARE(games.at(game_key).assets.single(AssetType::BOX_FRONT),
             QStringLiteral("file::/custom_assets/different_dir/whatever.png"));
}

void test_PegasusProvider::custom_directories()
{
    HashMap<QString, modeldata::Game> games;
    HashMap<QString, modeldata::Collection> collections;
    HashMap<QString, std::vector<QString>> collection_childs;

    QTest::ignoreMessage(QtInfoMsg, "Collections: found `:/custom_dirs/coll/collections.txt`");
    providers::pegasus::PegasusProvider provider({QStringLiteral(":/custom_dirs/coll")});
    provider.findLists(games, collections, collection_childs);
    provider.findStaticData(games, collections, collection_childs);

    QVERIFY(collections.size() == 2);
    QVERIFY(collections.count(QStringLiteral("x-files")) == 1);
    QVERIFY(collections.count(QStringLiteral("y-files")) == 1);
    QVERIFY(games.size() == 5);
    QVERIFY(collection_childs.at(QStringLiteral("x-files")).size() == 4);
    QVERIFY(collection_childs.at(QStringLiteral("y-files")).size() == 1);

    // NOTE: Apparently canonicalFilePath doesn't work with QRC resources...
    const QStringList xfiles {
        { ":/custom_dirs/coll/../a/mygame.x" },
        { ":/custom_dirs/coll/../b/mygame.x" },
        { ":/custom_dirs/c/mygame.x" },
        { ":/custom_dirs/coll/mygame.x" },
    };
    const QStringList yfiles {
        { ":/custom_dirs/coll/../b/mygame.y" },
    };

    for (const QString& game_key : collection_childs.at(QStringLiteral("x-files"))) {
        const modeldata::Game& game = games.at(game_key);
        QCOMPARE(xfiles.contains(game.fileinfo().canonicalFilePath()), true);
    }
    for (const QString& game_key : collection_childs.at(QStringLiteral("y-files"))) {
        const modeldata::Game& game = games.at(game_key);
        QCOMPARE(yfiles.contains(game.fileinfo().canonicalFilePath()), true);
    }
}*/


QTEST_MAIN(test_PegasusProvider)
#include "test_PegasusProvider.moc"
