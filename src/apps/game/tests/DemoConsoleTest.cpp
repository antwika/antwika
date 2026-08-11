#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <antwika/console/ConsoleState.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/game/DemoConsole.hpp"

using antwika::console::ConsoleState;
using antwika::geometry::GridCell;
using antwika::log::mocks::MockLogger;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Overlay;
using antwika::tilemap::saveMapFile;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::game::DemoCommands;
using antwika::game::landingLevel;
using antwika::game::Player;
using antwika::game::restingLevel;
using antwika::game::standableWalkable;
using antwika::game::topStandableWalkable;
using ::testing::NiceMock;

namespace
{
    GridCell cellAt(const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] TileMap mapOf(
        const std::uint32_t columns, const std::uint32_t rows)
    {
        return TileMap{MapHeader{.id = "demo"}, columns, rows};
    }

    void placeLone(TileMap &map, const GridCell cell, const Slab slab)
    {
        auto &column = map.at(cell);
        column.clear();
        (void)column.place(slab);
    }

    /**
     * @brief The last line the console printed.
     */
    [[nodiscard]] std::string lastLine(const ConsoleState &console)
    {
        return console.history().empty() ? std::string{}
                                         : console.history().back();
    }

    class DemoConsoleTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        TileMap map = mapOf(4, 4);
        Player player{};
        ConsoleState console;

        [[nodiscard]] DemoCommands commands()
        {
            return DemoCommands{map, player, logger};
        }

        void run(const std::string &line)
        {
            auto made = commands();
            made.execute(line, console);
        }
    };
}

TEST(StandableWalkableTest, StandableWalkable_RejectsAnAbsentSlab)
{
    auto map = mapOf(2, 2);

    EXPECT_FALSE(standableWalkable(map.at(cellAt(0, 0)), 5));
}

TEST(StandableWalkableTest, StandableWalkable_RejectsAWall)
{
    auto map = mapOf(2, 2);
    placeLone(
        map,
        cellAt(0, 0),
        Slab{.level = 0, .terrain = TerrainClass::Wall});

    EXPECT_FALSE(standableWalkable(map.at(cellAt(0, 0)), 0));
}

TEST(StandableWalkableTest, StandableWalkable_RejectsUnbridgedWater)
{
    auto map = mapOf(2, 2);
    placeLone(
        map,
        cellAt(0, 0),
        Slab{.level = 0, .terrain = TerrainClass::Water});

    EXPECT_FALSE(standableWalkable(map.at(cellAt(0, 0)), 0));
}

TEST(StandableWalkableTest, StandableWalkable_TakesBridgedWater)
{
    auto map = mapOf(2, 2);
    placeLone(
        map,
        cellAt(0, 0),
        Slab{
            .level = 0,
            .terrain = TerrainClass::Water,
            .overlay = Overlay::Bridge});

    EXPECT_TRUE(standableWalkable(map.at(cellAt(0, 0)), 0));
}

TEST(StandableWalkableTest, StandableWalkable_RejectsABlockedClearance)
{
    auto map = mapOf(2, 2);
    auto &column = map.at(cellAt(0, 0));
    (void)column.place(Slab{.level = 1});

    EXPECT_FALSE(standableWalkable(column, 0));
    EXPECT_TRUE(standableWalkable(column, 1));
}

TEST(TopStandableWalkableTest, TopStandableWalkable_TakesTheHighest)
{
    auto map = mapOf(2, 2);
    auto &column = map.at(cellAt(0, 0));
    (void)column.place(Slab{.level = 3});

    EXPECT_EQ(topStandableWalkable(column), 3);
}

TEST(TopStandableWalkableTest, TopStandableWalkable_SkipsAWallOnTop)
{
    auto map = mapOf(2, 2);
    auto &column = map.at(cellAt(0, 0));
    (void)column.place(
        Slab{.level = 3, .terrain = TerrainClass::Wall});

    EXPECT_EQ(topStandableWalkable(column), 0);
}

TEST(TopStandableWalkableTest, TopStandableWalkable_YieldsNothingOnAWall)
{
    auto map = mapOf(2, 2);
    placeLone(
        map,
        cellAt(0, 0),
        Slab{.level = 0, .terrain = TerrainClass::Wall});

    EXPECT_FALSE(
        topStandableWalkable(map.at(cellAt(0, 0))).has_value());
}

TEST(RestingLevelTest, RestingLevel_TakesTheTopWalkableSurface)
{
    auto map = mapOf(2, 2);
    auto &column = map.at(cellAt(0, 0));
    (void)column.place(Slab{.level = 2});

    EXPECT_EQ(restingLevel(column), 2);
}

TEST(RestingLevelTest, RestingLevel_FallsBackToTheTopSlab)
{
    auto map = mapOf(2, 2);
    placeLone(
        map,
        cellAt(0, 0),
        Slab{.level = 3, .terrain = TerrainClass::Wall});

    EXPECT_EQ(restingLevel(map.at(cellAt(0, 0))), 3);
}

TEST(RestingLevelTest, RestingLevel_FallsBackToZeroOnAnEmptyColumn)
{
    auto map = mapOf(2, 2);
    map.at(cellAt(0, 0)).clear();

    EXPECT_EQ(restingLevel(map.at(cellAt(0, 0))), 0);
}

TEST(LandingLevelTest, LandingLevel_StepsOntoTheLevelSurface)
{
    auto map = mapOf(3, 1);

    EXPECT_EQ(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0)), 0);
}

TEST(LandingLevelTest, LandingLevel_DropsToASurfaceBelow)
{
    auto map = mapOf(3, 1);
    placeLone(map, cellAt(0, 0), Slab{.level = 3});

    EXPECT_EQ(
        landingLevel(map, cellAt(0, 0), 3, cellAt(1, 0)), 0);
}

TEST(LandingLevelTest, LandingLevel_BlocksAStepIntoAWall)
{
    auto map = mapOf(3, 1);
    placeLone(
        map,
        cellAt(1, 0),
        Slab{.level = 0, .terrain = TerrainClass::Wall});

    EXPECT_FALSE(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0))
            .has_value());
}

TEST(LandingLevelTest, LandingLevel_BlocksAPlainStepUp)
{
    auto map = mapOf(3, 1);
    placeLone(map, cellAt(1, 0), Slab{.level = 1});

    EXPECT_FALSE(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0))
            .has_value());
}

TEST(LandingLevelTest, LandingLevel_ClimbsAStairOnTheTargetSide)
{
    auto map = mapOf(3, 1);
    placeLone(
        map,
        cellAt(1, 0),
        Slab{.level = 1, .terrain = TerrainClass::Stair});

    EXPECT_EQ(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0)), 1);
}

TEST(LandingLevelTest, LandingLevel_ClimbsOffAStairOnTheSourceSide)
{
    auto map = mapOf(3, 1);
    placeLone(
        map,
        cellAt(0, 0),
        Slab{.level = 0, .terrain = TerrainClass::Stair});
    placeLone(map, cellAt(1, 0), Slab{.level = 1});

    EXPECT_EQ(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0)), 1);
}

TEST(LandingLevelTest, LandingLevel_PrefersTheStairOverTheSurfaceBelow)
{
    auto map = mapOf(3, 1);
    auto &target = map.at(cellAt(1, 0));
    (void)target.place(
        Slab{.level = 1, .terrain = TerrainClass::Stair});

    EXPECT_EQ(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0)), 1);
}

TEST(LandingLevelTest, LandingLevel_BlocksAStepIntoAnEmptyColumn)
{
    auto map = mapOf(3, 1);
    map.at(cellAt(1, 0)).clear();

    EXPECT_FALSE(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0))
            .has_value());
}

TEST_F(DemoConsoleTest, Names_ListsEveryCommandItAnswers)
{
    auto made = commands();

    EXPECT_EQ(
        made.names(),
        (std::vector<std::string>{
            "help", "map", "tp", "pos", "palette"}));
}

TEST_F(DemoConsoleTest, Execute_PrintsEveryHelpLine)
{
    run("help");

    EXPECT_EQ(console.history().size(), 6U);
    EXPECT_EQ(lastLine(console), "quit - close the demo");
}

TEST_F(DemoConsoleTest, Execute_ReportsAnUnknownCommand)
{
    run("fly");

    EXPECT_EQ(lastLine(console), "unknown command: fly - try help");
}

TEST_F(DemoConsoleTest, Execute_PrintsThePlayerPosition)
{
    player.cell = cellAt(1, 2);
    player.level = 3;

    run("pos");

    EXPECT_EQ(lastLine(console), "player at 1,2 level 3");
}

TEST_F(DemoConsoleTest, Map_AsksForAFileWhenGivenNoArgument)
{
    run("map");

    EXPECT_EQ(lastLine(console), "map: name a file to load");
}

TEST_F(DemoConsoleTest, Map_ReportsAFileItCannotLoad)
{
    const ScratchDirectory scratch("democonsole.");

    run("map " + (scratch.path() / "absent.json").string());

    EXPECT_THAT(lastLine(console), ::testing::StartsWith("map: "));
    EXPECT_EQ(map.columns(), 4U);
}

TEST_F(DemoConsoleTest, Map_LoadsAFileAndSettlesThePlayer)
{
    const ScratchDirectory scratch("democonsole.");
    const auto where = scratch.path() / "loaded.json";
    auto stored = mapOf(2, 2);
    (void)stored.at(cellAt(1, 1)).place(Slab{.level = 2});
    saveMapFile(where, stored);

    player.cell = cellAt(3, 3);
    player.moveTicks = 7;

    run("map " + where.string());

    EXPECT_EQ(map.columns(), 2U);
    EXPECT_EQ(player.cell, cellAt(1, 1));
    EXPECT_EQ(player.level, 2);
    EXPECT_EQ(player.moveTicks, 0U);
    EXPECT_EQ(lastLine(console), "loaded " + where.string());
}

TEST_F(DemoConsoleTest, Tp_AsksForTwoNumbers)
{
    run("tp 1");

    EXPECT_EQ(lastLine(console), "tp: say tp <column> <row>");
}

TEST_F(DemoConsoleTest, Tp_RefusesAColumnThatIsNotANumber)
{
    run("tp east 2");

    EXPECT_EQ(lastLine(console), "tp: say tp <column> <row>");
}

TEST_F(DemoConsoleTest, Tp_RefusesARowWithTrailingText)
{
    run("tp 1 2x");

    EXPECT_EQ(lastLine(console), "tp: say tp <column> <row>");
}

TEST_F(DemoConsoleTest, Tp_MovesThePlayerToTheCell)
{
    player.moveTicks = 5;

    run("tp 2 3");

    EXPECT_EQ(player.cell, cellAt(2, 3));
    EXPECT_EQ(player.level, 0);
    EXPECT_EQ(player.moveTicks, 0U);
    EXPECT_EQ(lastLine(console), "teleported to 2,3");
}

TEST_F(DemoConsoleTest, Tp_ClampsToTheEdgeOfTheGrid)
{
    run("tp 99 99");

    EXPECT_EQ(player.cell, cellAt(3, 3));
    EXPECT_EQ(lastLine(console), "teleported to 3,3");
}

TEST_F(DemoConsoleTest, Tp_RefusesACellWithNoStandableSurface)
{
    placeLone(
        map,
        cellAt(1, 1),
        Slab{.level = 0, .terrain = TerrainClass::Wall});

    run("tp 1 1");

    EXPECT_EQ(
        lastLine(console), "tp: 1,1 has no standable surface");
    EXPECT_EQ(player.cell, cellAt(2, 4));
}

TEST_F(DemoConsoleTest, Palette_AsksForAChannelAndAColor)
{
    run("palette");

    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");
}

TEST_F(DemoConsoleTest, Palette_RefusesAnUnknownChannel)
{
    run("palette border #ff0000");

    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");
}

TEST_F(DemoConsoleTest, Palette_RefusesAColorOfTheWrongLength)
{
    run("palette ink #ff00");

    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");
}

TEST_F(DemoConsoleTest, Palette_RefusesANonHexDigit)
{
    run("palette ink #ff00zz");

    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");
}

TEST_F(DemoConsoleTest, Palette_SetsTheInkFromAHashedColor)
{
    run("palette ink #10a0FF");

    EXPECT_EQ(map.header().ink.red, 0x10);
    EXPECT_EQ(map.header().ink.green, 0xA0);
    EXPECT_EQ(map.header().ink.blue, 0xFF);
    EXPECT_EQ(lastLine(console), "palette ink set to #10a0FF");
}

TEST_F(DemoConsoleTest, Palette_SetsThePaperFromABareColor)
{
    run("palette paper 0b0c0d");

    EXPECT_EQ(map.header().paper.red, 0x0B);
    EXPECT_EQ(map.header().paper.blue, 0x0D);
}

TEST_F(DemoConsoleTest, Palette_LeavesTheOtherChannelAlone)
{
    const auto before = map.header().paper;

    run("palette ink #010203");

    EXPECT_EQ(map.header().paper, before);
}

TEST_F(DemoConsoleTest, Palette_KeepsTheSlabsAndEntities)
{
    (void)map.at(cellAt(1, 1))
        .place(Slab{.level = 2, .terrain = TerrainClass::Stair});
    map.addEntity(antwika::tilemap::Npc{
        .id = "keeper", .at = cellAt(1, 1), .level = 2});

    run("palette ink #010203");

    ASSERT_NE(map.at(cellAt(1, 1)).slabAt(2), nullptr);
    EXPECT_EQ(
        map.at(cellAt(1, 1)).slabAt(2)->terrain, TerrainClass::Stair);
    EXPECT_EQ(map.entities().size(), 1U);
}

TEST_F(DemoConsoleTest, Tp_TreatsATrailingRunOfSpacesAsNoArgument)
{
    run("tp    ");

    EXPECT_EQ(lastLine(console), "tp: say tp <column> <row>");
}

TEST_F(DemoConsoleTest, Palette_RefusesADigitBelowTheHexRange)
{
    run("palette ink #!!0000");

    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");
}

TEST_F(DemoConsoleTest, Palette_RefusesALowNibbleThatIsNotHex)
{
    run("palette ink #0z0000");

    EXPECT_EQ(
        lastLine(console),
        "palette: say palette <ink|paper> <#rrggbb>");
}

TEST(LandingLevelTest, LandingLevel_BlocksAStepUpOntoAWall)
{
    auto map = mapOf(3, 1);
    placeLone(
        map,
        cellAt(1, 0),
        Slab{.level = 1, .terrain = TerrainClass::Wall});

    EXPECT_FALSE(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0))
            .has_value());
}

TEST(LandingLevelTest, LandingLevel_ClimbsAStairFromAGapInTheSource)
{
    auto map = mapOf(3, 1);
    map.at(cellAt(0, 0)).clear();
    placeLone(
        map,
        cellAt(1, 0),
        Slab{.level = 1, .terrain = TerrainClass::Stair});

    EXPECT_EQ(
        landingLevel(map, cellAt(0, 0), 0, cellAt(1, 0)), 1);
}
