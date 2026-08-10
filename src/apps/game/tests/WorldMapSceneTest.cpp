#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Terrain.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapLayout.hpp"
#include "antwika/game/WorldMapScene.hpp"

namespace
{

    using antwika::game::Cell;
    using antwika::game::colorOf;
    using antwika::game::generateWorldMap;
    using antwika::game::kCityCount;
    using antwika::game::kWorldTileSize;
    using antwika::game::Terrain;
    using antwika::game::WorldMap;
    using antwika::game::worldSnapshotOf;
    using antwika::game::WorldMapScene;
    using antwika::game::WorldMapSnapshot;
    using antwika::game::worldTileRect;
    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::gfx::RectF;
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using ::testing::_;

    constexpr Size kCanvas{
        .width = 8 * kWorldTileSize, .height = 8 * kWorldTileSize};

    WorldMapSnapshot flatSnapshot()
    {
        WorldMapSnapshot snapshot;
        snapshot.width = 2;
        snapshot.height = 2;
        snapshot.tiles = {
            Terrain::Water,
            Terrain::Plains,
            Terrain::Forest,
            Terrain::Hills};
        snapshot.cities = {
            Cell{0, 0}, Cell{1, 0}, Cell{0, 1}, Cell{1, 1}};
        return snapshot;
    }

    TEST(WorldMapSceneTest, ColorOf_IsUniquePerTerrain)
    {
        const std::vector<Terrain> all{
            Terrain::Water,
            Terrain::Plains,
            Terrain::Forest,
            Terrain::Hills,
            Terrain::Mountain};
        for (std::size_t i = 0; i < all.size(); ++i)
        {
            for (std::size_t j = i + 1; j < all.size(); ++j)
            {
                EXPECT_NE(colorOf(all[i]), colorOf(all[j]));
            }
        }
    }

    TEST(WorldMapSceneTest, Draw_DrawsOneRectPerTile)
    {
        const WorldMapSnapshot snapshot = flatSnapshot();
        MockRenderer renderer;

        EXPECT_CALL(renderer, drawRect(_, _)).Times(kCityCount);
        EXPECT_CALL(
            renderer,
            drawRect(
                RectF{worldTileRect(kCanvas, 2, 2, Cell{0, 0})},
                colorOf(Terrain::Water)));
        EXPECT_CALL(
            renderer,
            drawRect(
                RectF{worldTileRect(kCanvas, 2, 2, Cell{1, 0})},
                colorOf(Terrain::Plains)));
        EXPECT_CALL(
            renderer,
            drawRect(
                RectF{worldTileRect(kCanvas, 2, 2, Cell{0, 1})},
                colorOf(Terrain::Forest)));
        EXPECT_CALL(
            renderer,
            drawRect(
                RectF{worldTileRect(kCanvas, 2, 2, Cell{1, 1})},
                colorOf(Terrain::Hills)));
        EXPECT_CALL(renderer, drawText(_, _, 1, _)).Times(kCityCount);

        WorldMapScene().draw(renderer, kCanvas, snapshot);
    }

    TEST(WorldMapSceneTest, WorldSnapshotOf_CarriesTerrainAndCities)
    {
        const WorldMap world = generateWorldMap({12, 10, 5});
        const WorldMapSnapshot snapshot = worldSnapshotOf(world);

        EXPECT_EQ(snapshot.width, world.width);
        EXPECT_EQ(snapshot.height, world.height);
        EXPECT_EQ(snapshot.tiles, world.tiles);
        for (std::size_t city = 0; city < kCityCount; ++city)
        {
            EXPECT_EQ(snapshot.cities[city], world.cityCell(city));
        }
    }

}

TEST(WorldMapSceneTest, ColorOf_AnswersWaterForAnythingUnrecognised)
{
    EXPECT_EQ(
        antwika::game::colorOf(
            static_cast<antwika::game::Terrain>(99)),
        antwika::game::colorOf(antwika::game::Terrain::Water));
}
