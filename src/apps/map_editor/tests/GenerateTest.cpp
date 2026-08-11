#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/Generate.hpp"
#include "antwika/map_editor/GenerationRules.hpp"

using antwika::geometry::GridCell;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::defaultGenerationRules;
using antwika::map_editor::EditorState;
using antwika::map_editor::generate;
using antwika::map_editor::generateTerrains;
using antwika::map_editor::kTerrainCount;
using antwika::map_editor::pinAll;
using antwika::map_editor::pinIndex;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    /**
     * @brief A map whose cells are all free for the solver.
     */
    [[nodiscard]] EditorState freeState(
        const std::uint32_t columns, const std::uint32_t rows)
    {
        EditorState state{.map = TileMap{MapHeader{}, columns, rows}};
        pinAll(state);
        state.pinned.assign(state.pinned.size(), false);

        return state;
    }
}

TEST(GenerateTest, GenerateTerrains_SolvesAWholeFreeMap)
{
    const auto state = freeState(4, 4);

    const auto terrains = generateTerrains(state, 1);

    ASSERT_TRUE(terrains.has_value());
    EXPECT_EQ(terrains->size(), 16U);
}

TEST(GenerateTest, GenerateTerrains_NeverPutsAStairOnAFreeCell)
{
    const auto state = freeState(6, 6);

    const auto terrains = generateTerrains(state, 3);

    ASSERT_TRUE(terrains.has_value());

    for (const auto terrain : *terrains)
    {
        EXPECT_NE(terrain, TerrainClass::Stair);
    }
}

TEST(GenerateTest, GenerateTerrains_KeepsThePinnedCellsTerrain)
{
    auto state = freeState(4, 4);
    state.pinned[pinIndex(state.map, cellAt(1, 1))] = true;
    state.map.at(cellAt(1, 1)).top()->terrain = TerrainClass::Stair;

    const auto terrains = generateTerrains(state, 5);

    ASSERT_TRUE(terrains.has_value());
    EXPECT_EQ(
        (*terrains)[pinIndex(state.map, cellAt(1, 1))],
        TerrainClass::Stair);
}

TEST(GenerateTest, GenerateTerrains_SolvesAroundAPinnedVoidCell)
{
    auto state = freeState(4, 4);
    state.pinned[pinIndex(state.map, cellAt(2, 2))] = true;
    state.map.at(cellAt(2, 2)).clear();

    const auto terrains = generateTerrains(state, 7);

    ASSERT_TRUE(terrains.has_value());
    EXPECT_EQ(terrains->size(), 16U);
}

TEST(GenerateTest, GenerateTerrains_SolvesAtAnActiveLevelAboveZero)
{
    auto state = freeState(3, 3);
    state.activeLevel = 2;

    const auto terrains = generateTerrains(state, 2);

    ASSERT_TRUE(terrains.has_value());
    EXPECT_EQ(terrains->size(), 9U);
}

TEST(GenerateTest, GenerateTerrains_IsTheSameForTheSameSeed)
{
    const auto state = freeState(5, 5);

    EXPECT_EQ(generateTerrains(state, 11), generateTerrains(state, 11));
}

TEST(GenerateTest, GenerateTerrains_YieldsNothingWithNoLegalTerrain)
{
    auto state = freeState(3, 3);

    for (auto &row : state.rules.allowed)
    {
        row.fill(false);
    }

    EXPECT_FALSE(generateTerrains(state, 1).has_value());
}

TEST(GenerateTest, GenerateTerrains_SolvesASingleCellMap)
{
    const auto state = freeState(1, 1);

    const auto terrains = generateTerrains(state, 1);

    ASSERT_TRUE(terrains.has_value());
    EXPECT_EQ(terrains->size(), 1U);
}

TEST(GenerateTest, Generate_WritesTheTerrainsAsOneUndoStep)
{
    NiceMock<MockLogger> logger;
    auto state = freeState(4, 4);

    EXPECT_CALL(logger, log(antwika::log::Level::Info, _)).Times(1);

    generate(state, logger);

    EXPECT_EQ(state.undoStack.size(), 1U);
    EXPECT_EQ(state.generateFailedTicks, 0U);
}

TEST(GenerateTest, Generate_StepsTheSeedExactlyOnce)
{
    NiceMock<MockLogger> logger;
    auto state = freeState(3, 3);
    const auto before = state.generateSeed;

    generate(state, logger);

    EXPECT_EQ(state.generateSeed, before + 1);
}

TEST(GenerateTest, Generate_RaisesTheNoticeWhenNothingSolves)
{
    NiceMock<MockLogger> logger;
    auto state = freeState(3, 3);

    for (auto &row : state.rules.allowed)
    {
        row.fill(false);
    }

    EXPECT_CALL(logger, log(antwika::log::Level::Warning, _)).Times(1);

    generate(state, logger);

    EXPECT_GT(state.generateFailedTicks, 0U);
    EXPECT_TRUE(state.undoStack.empty());
}

TEST(GenerateTest, Generate_RepinsAMapThatChangedShapeFirst)
{
    NiceMock<MockLogger> logger;
    auto state = freeState(3, 3);
    state.pinned.assign(2, false);

    generate(state, logger);

    EXPECT_EQ(state.pinned.size(), 9U);
}

TEST(GenerateTest, GenerateTerrains_TakesASeedThatScramblesToZero)
{
    const auto state = freeState(3, 3);

    const auto terrains = generateTerrains(state, 0x61C88647U);

    ASSERT_TRUE(terrains.has_value());
    EXPECT_EQ(terrains->size(), 9U);
}
