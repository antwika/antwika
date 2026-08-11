#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/mapcheck_cli/CheckMaps.hpp"

using antwika::geometry::GridCell;
using antwika::mapcheck_cli::checkMaps;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::saveMapFile;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;

namespace
{
    [[nodiscard]] TileMap walkableMap(
        const std::uint32_t columns, const std::uint32_t rows)
    {
        return TileMap{MapHeader{}, columns, rows};
    }

    /**
     * @brief Writes a map into the scratch directory.
     *
     * @return The path it was written to.
     */
    [[nodiscard]] std::filesystem::path put(
        const ScratchDirectory &scratch,
        const std::string &name,
        const TileMap &map)
    {
        const auto where = scratch.path() / (name + ".json");
        saveMapFile(where, map);

        return where;
    }
}

TEST(CheckMapsTest, CheckMaps_PassesACleanMapWithoutPrinting)
{
    const ScratchDirectory scratch("mapcheck.");
    std::ostringstream out;

    EXPECT_TRUE(
        checkMaps({put(scratch, "clean", walkableMap(3, 3))}, out));
    EXPECT_TRUE(out.str().empty());
}

TEST(CheckMapsTest, CheckMaps_PassesWithNoPathsAtAll)
{
    std::ostringstream out;

    EXPECT_TRUE(checkMaps({}, out));
    EXPECT_TRUE(out.str().empty());
}

TEST(CheckMapsTest, CheckMaps_ReportsAFileItCannotLoad)
{
    const ScratchDirectory scratch("mapcheck.");
    scratch.write("broken.json", "{ not json");
    std::ostringstream out;

    EXPECT_FALSE(
        checkMaps({scratch.path() / "broken.json"}, out));
    EXPECT_NE(out.str().find("broken:"), std::string::npos);
}

TEST(CheckMapsTest, CheckMaps_KeepsCheckingAfterAFileFailsToLoad)
{
    const ScratchDirectory scratch("mapcheck.");
    scratch.write("broken.json", "{ not json");
    const auto clean = put(scratch, "clean", walkableMap(3, 3));
    std::ostringstream out;

    EXPECT_FALSE(
        checkMaps({scratch.path() / "broken.json", clean}, out));
    EXPECT_NE(out.str().find("broken:"), std::string::npos);
    EXPECT_EQ(out.str().find("clean:"), std::string::npos);
}

TEST(CheckMapsTest, CheckMaps_NamesTheCellAndLevelOfAFinding)
{
    const ScratchDirectory scratch("mapcheck.");
    auto map = walkableMap(3, 3);

    map.addEntity(Transition{
        .id = "door",
        .at = GridCell{.column = 2, .row = 2},
        .level = 0,
        .targetMap = "absent",
        .targetEntry = "nowhere"});
    map.at(GridCell{.column = 1, .row = 1}).top()->terrain =
        TerrainClass::Wall;

    std::ostringstream out;

    EXPECT_FALSE(checkMaps({put(scratch, "holes", map)}, out));
    EXPECT_NE(out.str().find("holes: "), std::string::npos);
}

TEST(CheckMapsTest, CheckMaps_EntersFromTheFirstTransitionItFinds)
{
    const ScratchDirectory scratch("mapcheck.");
    auto map = walkableMap(3, 3);

    map.at(GridCell{.column = 1, .row = 1}).clear();
    map.addEntity(Transition{
        .id = "door",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0,
        .targetMap = "twin",
        .targetEntry = "back"});

    auto twin = walkableMap(3, 3);
    twin.addEntity(Transition{
        .id = "back",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0,
        .targetMap = "holes",
        .targetEntry = "door"});

    std::ostringstream out;

    EXPECT_TRUE(
        checkMaps(
            {put(scratch, "holes", map), put(scratch, "twin", twin)},
            out));
    EXPECT_TRUE(out.str().empty());
}

TEST(CheckMapsTest, CheckMaps_FallsBackToTheSecondCellWithoutATransition)
{
    const ScratchDirectory scratch("mapcheck.");
    auto map = walkableMap(3, 3);

    for (std::uint32_t row = 0; row < 3; ++row)
    {
        for (std::uint32_t column = 0; column < 3; ++column)
        {
            auto &held = map.at(GridCell{.column = column, .row = row});
            held.clear();
            (void)held.place(Slab{.level = 2});
        }
    }

    std::ostringstream out;

    EXPECT_TRUE(checkMaps({put(scratch, "raised", map)}, out));
    EXPECT_TRUE(out.str().empty());
}

TEST(CheckMapsTest, CheckMaps_FallsBackOnAGridTooSmallForThatCell)
{
    const ScratchDirectory scratch("mapcheck.");
    std::ostringstream out;

    EXPECT_FALSE(
        checkMaps({put(scratch, "tiny", walkableMap(1, 1))}, out));
    EXPECT_NE(out.str().find("tiny: "), std::string::npos);
}

TEST(CheckMapsTest, CheckMaps_FallsBackOnAnEmptyColumn)
{
    const ScratchDirectory scratch("mapcheck.");
    auto map = walkableMap(3, 3);

    map.at(GridCell{.column = 1, .row = 1}).clear();

    std::ostringstream out;

    EXPECT_FALSE(checkMaps({put(scratch, "hollow", map)}, out));
    EXPECT_NE(out.str().find("hollow: "), std::string::npos);
}

TEST(CheckMapsTest, CheckMaps_ReportsATransitionWithoutACounterpart)
{
    const ScratchDirectory scratch("mapcheck.");
    auto map = walkableMap(3, 3);

    map.addEntity(Transition{
        .id = "door",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0,
        .targetMap = "absent",
        .targetEntry = "nowhere"});

    std::ostringstream out;

    EXPECT_FALSE(checkMaps({put(scratch, "lonely", map)}, out));
    EXPECT_NE(out.str().find("lonely"), std::string::npos);
}

TEST(CheckMapsTest, CheckMaps_SkipsEntitiesThatAreNotTransitions)
{
    const ScratchDirectory scratch("mapcheck.");
    auto map = walkableMap(3, 3);

    map.at(GridCell{.column = 1, .row = 1}).clear();
    map.addEntity(Npc{
        .id = "keeper",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0});

    std::ostringstream out;

    EXPECT_FALSE(checkMaps({put(scratch, "hollow", map)}, out));
    EXPECT_NE(out.str().find("hollow: "), std::string::npos);
}

TEST(CheckMapsTest, CheckMaps_FallsBackOnAGridWithTooFewRows)
{
    const ScratchDirectory scratch("mapcheck.");
    std::ostringstream out;

    EXPECT_FALSE(
        checkMaps({put(scratch, "flat", walkableMap(3, 1))}, out));
    EXPECT_NE(out.str().find("flat: "), std::string::npos);
}
