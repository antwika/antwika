#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/autotile/DrawPlan.hpp>
#include <antwika/autotile/Metrics.hpp>
#include <antwika/autotile/TileDraw.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/geometry/Point.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>

using antwika::autotile::buildDrawPlan;
using antwika::autotile::DrawKind;
using antwika::autotile::DrawPlan;
using antwika::autotile::kHalfTile;
using antwika::autotile::kLevelRise;
using antwika::autotile::kUnit;
using antwika::autotile::TileDraw;
using antwika::autotile::TilesetBindings;
using antwika::geometry::GridCell;
using antwika::geometry::Point;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Overlay;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::internSocket;
using antwika::tileset::kEdgeSocket;
using antwika::tileset::kOpenSocket;
using antwika::tileset::SocketId;
using antwika::tileset::Sprite;
using antwika::tileset::Tileset;

namespace
{
    constexpr std::size_t kNorth = 0;
    constexpr std::size_t kEast = 1;
    constexpr std::size_t kSouth = 2;
    constexpr std::size_t kWest = 3;

    class Sets final
    {
    public:
        Sets()
        {
            for (std::size_t at = 0; at < sets_.size(); ++at)
            {
                sets_[at].terrain = static_cast<TerrainClass>(at);
            }
        }

        Tileset &of(const TerrainClass terrain)
        {
            return sets_[antwika::enums::index(terrain)];
        }

        [[nodiscard]] TilesetBindings bindings() const
        {
            TilesetBindings out{};

            for (std::size_t at = 0; at < sets_.size(); ++at)
            {
                out.byTerrain[at] = &sets_[at];
            }

            return out;
        }

    private:
        std::array<
            Tileset,
            antwika::enums::kCount<TerrainClass>>
            sets_{};
    };

    TileMap mapOf(const std::uint32_t columns, const std::uint32_t rows)
    {
        return TileMap(MapHeader{.id = "drawplan"}, columns, rows);
    }

    Sprite &addBase(
        Tileset &set, const std::array<SocketId, 4> sockets)
    {
        auto &sprite = addSprite(set, 0);
        sprite.sockets = sockets;

        return sprite;
    }

    /**
     * @brief Adds the sixteen shapes a border mask can ask for.
     *
     * Ensures: the sprite at index mask carries an edge socket on
     *          exactly the sides whose bit is set, north first.
     */
    void addWangSet(Tileset &set)
    {
        for (std::uint32_t mask = 0; mask < 16; ++mask)
        {
            std::array<SocketId, 4> sockets{};

            for (std::size_t side = 0; side < 4; ++side)
            {
                sockets[side] = ((mask >> side) & 1U) != 0
                                    ? kEdgeSocket
                                    : kOpenSocket;
            }

            (void)addBase(set, sockets);
        }
    }

    std::size_t countOf(const DrawPlan &plan, const DrawKind kind)
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            plan,
            [kind](const TileDraw &draw)
            { return draw.kind == kind; }));
    }

    std::size_t countOfRow(
        const DrawPlan &plan, const std::uint16_t atlasRow)
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            plan,
            [atlasRow](const TileDraw &draw)
            {
                return draw.kind == DrawKind::Sprite
                       && draw.atlasRow == atlasRow;
            }));
    }

    const TileDraw *spriteAt(const DrawPlan &plan, const Point screen)
    {
        const auto found = std::ranges::find_if(
            plan,
            [screen](const TileDraw &draw)
            {
                return draw.kind == DrawKind::Sprite
                       && draw.screen == screen;
            });

        return found == plan.end() ? nullptr : &*found;
    }

    Point pointAt(const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }

    /**
     * @brief Finds the first draw a predicate accepts.
     *
     * @return Its index, or the plan's size when none matches.
     */
    template <typename Pred>
    std::size_t firstIndexOf(const DrawPlan &plan, Pred accepts)
    {
        for (std::size_t at = 0; at < plan.size(); ++at)
        {
            if (accepts(plan[at]))
            {
                return at;
            }
        }

        return plan.size();
    }

    /**
     * @brief Finds the last draw a predicate accepts.
     *
     * @return Its index, or the plan's size when none matches.
     */
    template <typename Pred>
    std::size_t lastIndexOf(const DrawPlan &plan, Pred accepts)
    {
        auto found = plan.size();

        for (std::size_t at = 0; at < plan.size(); ++at)
        {
            if (accepts(plan[at]))
            {
                found = at;
            }
        }

        return found;
    }

    const TileDraw *decorAt(const DrawPlan &plan, const Point screen)
    {
        const auto found = std::ranges::find_if(
            plan,
            [screen](const TileDraw &draw)
            {
                return draw.kind == DrawKind::Sprite
                       && draw.atlasRow >= 16
                       && draw.screen == screen;
            });

        return found == plan.end() ? nullptr : &*found;
    }

    /**
     * @brief Builds a decor layer whose sockets chain east and south.
     *
     * Ensures: sprite 0 stands alone, 1 and 2 pair west to east, and
     *          3 and 4 pair north to south.
     */
    void addChainingDecor(Tileset &set, const std::uint8_t density)
    {
        auto &layer = addLayer(set, "decor");
        layer.density = density;

        const auto link = internSocket(set, "link");
        const std::array<std::array<SocketId, 4>, 5> shapes{
            {{kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
             {kOpenSocket, link, kOpenSocket, kOpenSocket},
             {kOpenSocket, kOpenSocket, kOpenSocket, link},
             {kOpenSocket, kOpenSocket, link, kOpenSocket},
             {link, kOpenSocket, kOpenSocket, kOpenSocket}}};

        for (const auto &shape : shapes)
        {
            auto &sprite = addSprite(set, 1);
            sprite.sockets = shape;

            for (std::uint32_t base = 0; base < 16; ++base)
            {
                sprite.on.push_back(base);
            }
        }
    }

    bool isFloorBase(const TileDraw &draw)
    {
        return draw.kind == DrawKind::Sprite
               && draw.terrain == TerrainClass::Floor
               && draw.atlasRow < 16;
    }

    bool isFloorDecor(const TileDraw &draw)
    {
        return draw.kind == DrawKind::Sprite
               && draw.terrain == TerrainClass::Floor
               && draw.atlasRow >= 16;
    }
}

TEST(DrawPlanTest, BuildDrawPlan_YieldsNoDrawsForABareMap)
{
    const auto map = mapOf(1, 1);
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_TRUE(plan.empty());
}

TEST(DrawPlanTest, BuildDrawPlan_DrawsAWallRimUnderTheTopmostSlab)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 1});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 1, 0, sets.bindings());

    ASSERT_EQ(plan.size(), 2U);
    EXPECT_EQ(plan[0].kind, DrawKind::WallRim);
    EXPECT_EQ(plan[0].terrain, TerrainClass::Cliff);
    EXPECT_EQ(plan[0].screen, pointAt(0, kUnit - kLevelRise));
    EXPECT_EQ(plan[1].screen, pointAt(kHalfTile, kUnit - kLevelRise));
}

TEST(DrawPlanTest, BuildDrawPlan_DrawsAWallBandUnderACoveredSlab)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 1});
    (void)map.at(GridCell{}).place(Slab{.level = 2});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 2, 0, sets.bindings());

    ASSERT_EQ(plan.size(), 4U);
    EXPECT_EQ(plan[0].kind, DrawKind::WallBand);
    EXPECT_EQ(plan[1].kind, DrawKind::WallBand);
    EXPECT_EQ(plan[2].kind, DrawKind::WallRim);
    EXPECT_EQ(plan[3].kind, DrawKind::WallRim);
}

TEST(DrawPlanTest, BuildDrawPlan_OmitsAFaceTheSouthNeighbourHides)
{
    auto map = mapOf(1, 2);
    (void)map.at(GridCell{.column = 0, .row = 0}).place(Slab{.level = 1});
    (void)map.at(GridCell{.column = 0, .row = 1}).place(Slab{.level = 1});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 1, 0, sets.bindings());

    ASSERT_EQ(plan.size(), 2U);
    EXPECT_EQ(plan[0].screen.y, 2 * kUnit - kLevelRise);
}

TEST(DrawPlanTest, BuildDrawPlan_OmitsAFaceAtALevelTheColumnSkips)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 2});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 2, 0, sets.bindings());

    ASSERT_EQ(plan.size(), 2U);
    EXPECT_EQ(plan[0].screen.y, kUnit - 2 * kLevelRise);
}

TEST(DrawPlanTest, BuildDrawPlan_SpansLevelsBelowZero)
{
    auto map = mapOf(1, 2);
    auto &sunken = map.at(GridCell{.column = 0, .row = 0});
    sunken.clear();
    (void)sunken.place(Slab{.level = -2});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    ASSERT_EQ(plan.size(), 2U);
    EXPECT_EQ(plan[0].kind, DrawKind::WallRim);
    EXPECT_EQ(plan[0].screen.y, kUnit + 2 * kLevelRise);
}

TEST(DrawPlanTest, BuildDrawPlan_OmitsTheFacesOfACutAwayColumn)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 1});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_TRUE(plan.empty());
}

TEST(DrawPlanTest, BuildDrawPlan_DrawsAQuadOfDeckOverABridgeSlab)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(
        Slab{.level = 0,
             .terrain = TerrainClass::Water,
             .overlay = Overlay::Bridge});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    ASSERT_EQ(plan.size(), 4U);
    EXPECT_EQ(countOf(plan, DrawKind::BridgeDeck), 4U);
    EXPECT_EQ(plan[0].terrain, TerrainClass::Water);
    EXPECT_EQ(plan[0].screen, pointAt(0, 0));
    EXPECT_EQ(plan[1].screen, pointAt(kHalfTile, 0));
    EXPECT_EQ(plan[2].screen, pointAt(0, kHalfTile));
    EXPECT_EQ(plan[3].screen, pointAt(kHalfTile, kHalfTile));
}

TEST(DrawPlanTest, BuildDrawPlan_OmitsTheDeckOfACutAwayBridge)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(
        Slab{.level = 1, .overlay = Overlay::Bridge});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::BridgeDeck), 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_KeepsTheDeckBelowTheCutAwayLevel)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(
        Slab{.level = 0, .overlay = Overlay::Bridge});
    (void)map.at(GridCell{}).place(Slab{.level = 2});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 1, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::BridgeDeck), 4U);
}

TEST(DrawPlanTest, BuildDrawPlan_KeepsTheShadeBelowTheCutAwayLevel)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 0, .light = 200});
    (void)map.at(GridCell{}).place(Slab{.level = 1, .light = 100});
    (void)map.at(GridCell{}).place(Slab{.level = 3, .light = 100});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 1, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Shade), 4U);
}

TEST(DrawPlanTest, BuildDrawPlan_ShadesASlabDimmerThanTheThreshold)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(
        Slab{.level = 0, .terrain = TerrainClass::Path, .light = 191});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    ASSERT_EQ(plan.size(), 4U);
    EXPECT_EQ(countOf(plan, DrawKind::Shade), 4U);
    EXPECT_EQ(plan[0].terrain, TerrainClass::Path);
}

TEST(DrawPlanTest, BuildDrawPlan_ShadesTwiceBelowTheDenseThreshold)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 0, .light = 95});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Shade), 8U);
}

TEST(DrawPlanTest, BuildDrawPlan_LeavesALitSlabUnshaded)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 0, .light = 192});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Shade), 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_OmitsTheShadeOfACutAwayColumn)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 1, .light = 0});
    const Sets sets;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Shade), 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_DrawsOneBasePerLatticeCellOfARegion)
{
    const auto map = mapOf(2, 2);
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Sprite), 16U);
}

TEST(DrawPlanTest, BuildDrawPlan_FacesEdgeSocketsOutsideTheRegion)
{
    const auto map = mapOf(1, 1);
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    const auto *northWest = spriteAt(plan, pointAt(0, 0));
    const auto *southEast =
        spriteAt(plan, pointAt(kHalfTile, kHalfTile));

    ASSERT_NE(northWest, nullptr);
    ASSERT_NE(southEast, nullptr);
    EXPECT_EQ(northWest->atlasRow, (1U << kNorth) | (1U << kWest));
    EXPECT_EQ(southEast->atlasRow, (1U << kEast) | (1U << kSouth));
}

TEST(DrawPlanTest, BuildDrawPlan_PlacesLatticeCellsEightPixelsApart)
{
    const auto map = mapOf(1, 1);
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_NE(spriteAt(plan, pointAt(0, 0)), nullptr);
    EXPECT_NE(spriteAt(plan, pointAt(kHalfTile, 0)), nullptr);
    EXPECT_NE(spriteAt(plan, pointAt(0, kHalfTile)), nullptr);
    EXPECT_NE(spriteAt(plan, pointAt(kHalfTile, kHalfTile)), nullptr);
}

TEST(DrawPlanTest, BuildDrawPlan_RaisesASurfaceByItsLevel)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 3});
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 3, 0, sets.bindings());

    EXPECT_NE(spriteAt(plan, pointAt(0, -3 * kLevelRise)), nullptr);
}

TEST(DrawPlanTest, BuildDrawPlan_OmitsASurfaceCoveredByASlabAbove)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 1});
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 1, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Sprite), 4U);
    EXPECT_NE(spriteAt(plan, pointAt(0, -kLevelRise)), nullptr);
}

TEST(DrawPlanTest, BuildDrawPlan_OmitsTheSurfaceOfACutAwayColumn)
{
    auto map = mapOf(1, 1);
    (void)map.at(GridCell{}).place(Slab{.level = 1});
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Sprite), 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_OrdersTerrainsWaterBeforeFloor)
{
    auto map = mapOf(2, 1);
    (void)map.at(GridCell{.column = 0, .row = 0})
        .place(Slab{.level = 0, .terrain = TerrainClass::Water});
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));
    addWangSet(sets.of(TerrainClass::Water));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    const auto firstFloor = firstIndexOf(
        plan,
        [](const TileDraw &draw)
        { return draw.terrain == TerrainClass::Floor; });
    const auto lastWater = lastIndexOf(
        plan,
        [](const TileDraw &draw)
        { return draw.terrain == TerrainClass::Water; });

    ASSERT_LT(firstFloor, plan.size());
    ASSERT_LT(lastWater, plan.size());
    EXPECT_LT(lastWater, firstFloor);
}

TEST(DrawPlanTest, BuildDrawPlan_SkipsATerrainWithoutBaseSprites)
{
    const auto map = mapOf(1, 1);
    Sets sets;
    addWangSet(sets.of(TerrainClass::Water));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Sprite), 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_HoldsAStillSpriteOnItsFirstFrame)
{
    const auto map = mapOf(1, 1);
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 600, sets.bindings());

    for (const auto &draw : plan)
    {
        EXPECT_EQ(draw.frame, 0);
    }
}

TEST(DrawPlanTest, BuildDrawPlan_AdvancesAnAnimatedSpriteWithTheClock)
{
    const auto map = mapOf(1, 1);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);

    for (auto &sprite : set.layers[0].sprites)
    {
        sprite.frameCount = 3;
    }

    const auto atStart =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());
    const auto atOnePeriod =
        buildDrawPlan(map, GridCell{}, 0, 30, sets.bindings());
    const auto atThreePeriods =
        buildDrawPlan(map, GridCell{}, 0, 90, sets.bindings());

    ASSERT_FALSE(atStart.empty());
    EXPECT_EQ(atStart[0].frame, 0);
    EXPECT_EQ(atOnePeriod[0].frame, 1);
    EXPECT_EQ(atThreePeriods[0].frame, 0);
}

TEST(DrawPlanTest, BuildDrawPlan_YieldsTheSamePlanForTheSameInputs)
{
    const auto map = mapOf(3, 3);
    Sets sets;
    addWangSet(sets.of(TerrainClass::Floor));

    const auto first =
        buildDrawPlan(map, GridCell{}, 0, 7, sets.bindings());
    const auto second =
        buildDrawPlan(map, GridCell{}, 0, 7, sets.bindings());

    EXPECT_EQ(first, second);
}

TEST(DrawPlanTest, BuildDrawPlan_SharesCellsOutInProportionToWeight)
{
    const auto map = mapOf(24, 24);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    auto &light = addBase(
        set, {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket});
    light.weight = 1;
    auto &heavy = addBase(
        set, {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket});
    heavy.weight = 15;

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    const auto lightCount = countOfRow(plan, 0);
    const auto heavyCount = countOfRow(plan, 1);

    EXPECT_GT(lightCount, 0U);
    EXPECT_GT(heavyCount, lightCount * 5);
}

TEST(DrawPlanTest, BuildDrawPlan_KeepsASpriteWhoseNeighboursAllClash)
{
    const auto map = mapOf(1, 1);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    const auto north = internSocket(set, "n");
    const auto east = internSocket(set, "e");
    const auto south = internSocket(set, "s");
    const auto west = internSocket(set, "w");
    (void)addBase(set, {north, east, south, west});

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Sprite), 4U);
}

TEST(DrawPlanTest, BuildDrawPlan_MatchesSocketsAcrossAdjoiningCells)
{
    const auto map = mapOf(4, 4);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    for (std::int32_t row = 0; row < 8; ++row)
    {
        for (std::int32_t column = 1; column < 8; ++column)
        {
            const auto *here = spriteAt(
                plan,
                pointAt(column * kHalfTile, row * kHalfTile));
            const auto *left = spriteAt(
                plan,
                pointAt((column - 1) * kHalfTile, row * kHalfTile));

            ASSERT_NE(here, nullptr);
            ASSERT_NE(left, nullptr);

            const auto &sprites = set.layers[0].sprites;

            EXPECT_EQ(
                sprites[here->atlasRow].sockets[kWest],
                sprites[left->atlasRow].sockets[kEast]);
        }
    }
}

TEST(DrawPlanTest, BuildDrawPlan_DrawsDecorOnlyWhereABaseSits)
{
    auto map = mapOf(2, 2);
    (void)map.at(GridCell{.column = 1, .row = 1})
        .place(Slab{.level = 0, .terrain = TerrainClass::Water});
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    auto &layer = addLayer(set, "decor");
    layer.density = 255;

    for (std::uint32_t base = 0; base < 16; ++base)
    {
        auto &sprite = addSprite(set, 1);
        sprite.on.push_back(base);
    }

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    const auto floorCells = std::ranges::count_if(plan, isFloorBase);
    const auto decorCells = std::ranges::count_if(plan, isFloorDecor);

    EXPECT_EQ(floorCells, 12);
    EXPECT_EQ(decorCells, 12);
}

TEST(DrawPlanTest, BuildDrawPlan_DrawsDecorAfterTheBaseOfItsTerrain)
{
    const auto map = mapOf(2, 2);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    auto &layer = addLayer(set, "decor");
    layer.density = 255;

    for (std::uint32_t base = 0; base < 16; ++base)
    {
        auto &sprite = addSprite(set, 1);
        sprite.on.push_back(base);
    }

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    const auto lastBase = lastIndexOf(plan, isFloorBase);
    const auto firstDecor = firstIndexOf(plan, isFloorDecor);

    ASSERT_LT(lastBase, plan.size());
    ASSERT_LT(firstDecor, plan.size());
    EXPECT_LT(lastBase, firstDecor);
}

TEST(DrawPlanTest, BuildDrawPlan_SkipsDecorThatNamesNoBaseItSitsOn)
{
    const auto map = mapOf(2, 2);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    auto &layer = addLayer(set, "decor");
    layer.density = 255;
    (void)addSprite(set, 1);

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOfRow(plan, 16), 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_SkipsADecorLayerWithoutSprites)
{
    const auto map = mapOf(2, 2);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    (void)addLayer(set, "decor");

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOf(plan, DrawKind::Sprite), 16U);
}

TEST(DrawPlanTest, BuildDrawPlan_ScattersNoDecorAtZeroDensity)
{
    const auto map = mapOf(4, 4);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    auto &layer = addLayer(set, "decor");
    layer.density = 0;

    for (std::uint32_t base = 0; base < 16; ++base)
    {
        auto &sprite = addSprite(set, 1);
        sprite.on.push_back(base);
    }

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    EXPECT_EQ(countOfRow(plan, 16), 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_ScattersSomeDecorAtAMiddlingDensity)
{
    const auto map = mapOf(8, 8);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    auto &layer = addLayer(set, "decor");
    layer.density = 128;

    for (std::uint32_t base = 0; base < 16; ++base)
    {
        auto &sprite = addSprite(set, 1);
        sprite.on.push_back(base);
    }

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());

    const auto decor = std::ranges::count_if(
        plan,
        [](const TileDraw &draw) { return draw.atlasRow >= 16; });

    EXPECT_GT(decor, 0);
    EXPECT_LT(decor, 256);
}

TEST(DrawPlanTest, BuildDrawPlan_MatchesDecorSocketsWestToEast)
{
    const auto map = mapOf(4, 4);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    addChainingDecor(set, 255);

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());
    const auto &sprites = set.layers[1].sprites;
    std::size_t pairs = 0;

    for (std::int32_t row = 0; row < 8; ++row)
    {
        for (std::int32_t column = 1; column < 8; ++column)
        {
            const auto *here = decorAt(
                plan, pointAt(column * kHalfTile, row * kHalfTile));
            const auto *left = decorAt(
                plan,
                pointAt((column - 1) * kHalfTile, row * kHalfTile));

            if (here == nullptr || left == nullptr)
            {
                continue;
            }

            ++pairs;
            EXPECT_EQ(
                sprites[here->atlasRow - 16].sockets[kWest],
                sprites[left->atlasRow - 16].sockets[kEast]);
        }
    }

    EXPECT_GT(pairs, 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_MatchesDecorSocketsNorthToSouth)
{
    const auto map = mapOf(4, 4);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    addChainingDecor(set, 255);

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());
    const auto &sprites = set.layers[1].sprites;
    std::size_t pairs = 0;

    for (std::int32_t row = 1; row < 8; ++row)
    {
        for (std::int32_t column = 0; column < 8; ++column)
        {
            const auto *here = decorAt(
                plan, pointAt(column * kHalfTile, row * kHalfTile));
            const auto *above = decorAt(
                plan,
                pointAt(column * kHalfTile, (row - 1) * kHalfTile));

            if (here == nullptr || above == nullptr)
            {
                continue;
            }

            ++pairs;
            EXPECT_EQ(
                sprites[here->atlasRow - 16].sockets[kNorth],
                sprites[above->atlasRow - 16].sockets[kSouth]);
        }
    }

    EXPECT_GT(pairs, 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_ClosesDecorSocketsAtTheRegionEdge)
{
    const auto map = mapOf(4, 4);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    addChainingDecor(set, 255);

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());
    const auto &sprites = set.layers[1].sprites;
    std::size_t edges = 0;

    for (std::int32_t along = 0; along < 8; ++along)
    {
        const auto *east =
            decorAt(plan, pointAt(7 * kHalfTile, along * kHalfTile));
        const auto *south =
            decorAt(plan, pointAt(along * kHalfTile, 7 * kHalfTile));

        if (east != nullptr)
        {
            ++edges;
            EXPECT_EQ(
                sprites[east->atlasRow - 16].sockets[kEast],
                kOpenSocket);
        }

        if (south != nullptr)
        {
            ++edges;
            EXPECT_EQ(
                sprites[south->atlasRow - 16].sockets[kSouth],
                kOpenSocket);
        }
    }

    EXPECT_GT(edges, 0U);
}

TEST(DrawPlanTest, BuildDrawPlan_PlacesForcedDecorBelowTheDensity)
{
    const auto map = mapOf(8, 8);
    Sets sets;
    auto &set = sets.of(TerrainClass::Floor);
    addWangSet(set);
    addChainingDecor(set, 40);

    const auto plan =
        buildDrawPlan(map, GridCell{}, 0, 0, sets.bindings());
    const auto &sprites = set.layers[1].sprites;
    std::size_t forced = 0;

    for (std::int32_t row = 0; row < 16; ++row)
    {
        for (std::int32_t column = 1; column < 16; ++column)
        {
            const auto *left = decorAt(
                plan,
                pointAt((column - 1) * kHalfTile, row * kHalfTile));

            if (left == nullptr
                || sprites[left->atlasRow - 16].sockets[kEast]
                       == kOpenSocket)
            {
                continue;
            }

            ++forced;
            EXPECT_NE(
                decorAt(
                    plan,
                    pointAt(column * kHalfTile, row * kHalfTile)),
                nullptr);
        }
    }

    EXPECT_GT(forced, 0U);
}
