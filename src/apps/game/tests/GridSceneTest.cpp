#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::game::BuildingKind;
using antwika::game::buildingTile;
using antwika::game::BuildingView;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::cellToScreen;
using antwika::game::Direction;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::groundTile;
using antwika::game::linkBit;
using antwika::game::roadTile;
using antwika::game::SceneSnapshot;
using antwika::game::stockBarBounds;
using antwika::game::stockFillBounds;
using antwika::game::walkerTile;
using antwika::game::walkerTint;
using antwika::game::WalkerKind;
using antwika::game::WalkerView;
using antwika::gfx::Color;
using antwika::gfx::ITexture;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 640, .height = 480};

    constexpr Color kUntinted{
        .red = 255, .green = 255, .blue = 255, .alpha = 255};

    // Records the blits, so the picture can be inspected as a whole.
    // Otherwise it could only be asserted call by call.
    class RecordingRenderer : public NiceMock<MockRenderer>
    {
    public:
        RecordingRenderer()
        {
            ON_CALL(*this, drawTexture(_, _, _, _))
                .WillByDefault(
                    [this](
                        const ITexture &texture,
                        Rect source,
                        Rect destination,
                        Color tint)
                    {
                        blits.push_back(
                            Blit{&texture, source, destination, tint});
                    });

            ON_CALL(*this, drawRect(_, _))
                .WillByDefault(
                    [this](Rect rect, Color color)
                    {
                        fills.push_back(Fill{rect, color});
                    });
        }

        struct Blit
        {
            const ITexture *texture;
            Rect source;
            Rect destination;
            Color tint;
        };

        struct Fill
        {
            Rect rect;
            Color color;
        };

        [[nodiscard]] std::size_t blitsOf(Rect source) const
        {
            std::size_t count = 0;

            for (const auto &blit : blits)
            {
                if (blit.source == source)
                {
                    ++count;
                }
            }

            return count;
        }

        std::vector<Blit> blits;
        std::vector<Fill> fills;
    };

    [[nodiscard]] SceneSnapshot snapshot(
        Camera camera,
        GridExtent extent,
        std::vector<Cell> paths = {},
        std::vector<WalkerView> walkers = {},
        std::vector<BuildingView> buildings = {})
    {
        return SceneSnapshot{
            .camera = camera,
            .extent = extent,
            .paths = std::move(paths),
            .walkers = std::move(walkers),
            .buildings = std::move(buildings)};
    }

    // Every building kind, so a table-driven test cannot forget one.
    constexpr std::array<BuildingKind, 5> kEveryBuildingKind{
        BuildingKind::House,
        BuildingKind::FoodSource,
        BuildingKind::WaterSource,
        BuildingKind::FireStation,
        BuildingKind::ArchitectPost};

    // Every walker kind, for the same reason.
    constexpr std::array<WalkerKind, 4> kEveryWalkerKind{
        WalkerKind::Food,
        WalkerKind::Water,
        WalkerKind::Fireman,
        WalkerKind::Architect};
} // namespace

class GridSceneTest : public ::testing::Test
{
protected:
    RecordingRenderer renderer;
    NiceMock<MockTexture> atlas;
    GridScene scene;
};

TEST_F(GridSceneTest, Draw_ClearsBeforeLayingAnyGround)
{
    MockRenderer strict;
    const InSequence order;

    EXPECT_CALL(strict, clear(_));
    EXPECT_CALL(strict, drawTexture(_, _, _, _)).Times(AnyNumber());

    scene.draw(
        strict,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 2, .height = 2}),
        atlas);
}

TEST_F(GridSceneTest, Draw_LaysOneGroundTilePerVisibleCell)
{
    constexpr GridExtent extent{.width = 3, .height = 3};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(Camera(Point{.x = 300, .y = 40}, 2), extent),
        atlas);

    EXPECT_EQ(renderer.blitsOf(groundTile()), 3U * 3U);
    EXPECT_EQ(renderer.blits.size(), 3U * 3U);
}

TEST_F(GridSceneTest, Draw_BlitsEachTileIntoItsOwnCellsBounds)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            camera,
            GridExtent{},
            {where},
            {WalkerView{.at = where, .facing = Direction::East}}),
        atlas);

    ASSERT_EQ(renderer.blits.size(), 2U);

    for (const auto &blit : renderer.blits)
    {
        EXPECT_EQ(blit.destination, cellBounds(where, camera));
    }
}

// The art carries every colour the ground and the roads have.
// A tint there would mean the picture was decided in two places.
// A walker is the exception, and has a test of its own below.
TEST_F(GridSceneTest, Draw_BlitsTheAtlasItIsGivenAndTintsNothing)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 2, .height = 2},
            {Cell{.x = 0, .y = 0}},
            {},
            {BuildingView{.at = Cell{.x = 1, .y = 1}}}),
        atlas);

    ASSERT_FALSE(renderer.blits.empty());

    for (const auto &blit : renderer.blits)
    {
        EXPECT_EQ(blit.texture, &atlas);
        EXPECT_EQ(blit.tint, kUntinted);
    }
}

TEST_F(GridSceneTest, Draw_DrawsAnIsolatedRoadWithNoLinksAtAll)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{},
            {Cell{.x = 1, .y = 1}}),
        atlas);

    EXPECT_EQ(renderer.blitsOf(roadTile(0)), 1U);
}

TEST_F(GridSceneTest, Draw_ChoosesARoadTileFromTheNeighboursItHas)
{
    // A straight west-to-east run, so the middle cell is a through road.
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{},
            {Cell{.x = 0, .y = 1},
             Cell{.x = 1, .y = 1},
             Cell{.x = 2, .y = 1}}),
        atlas);

    EXPECT_EQ(
        renderer.blitsOf(
            roadTile(linkBit(Direction::East) | linkBit(Direction::West))),
        1U);

    // And each end is a road with the one link back into the run.
    EXPECT_EQ(renderer.blitsOf(roadTile(linkBit(Direction::East))), 1U);
    EXPECT_EQ(renderer.blitsOf(roadTile(linkBit(Direction::West))), 1U);
}

// North and south have to reach the mask too, not only east and west.
TEST_F(GridSceneTest, Draw_ChoosesTheJunctionTileWhereFourRoadsMeet)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{},
            // Ascending, as PathIndex's set hands them over.
            {Cell{.x = 0, .y = 1},
             Cell{.x = 1, .y = 0},
             Cell{.x = 1, .y = 1},
             Cell{.x = 1, .y = 2},
             Cell{.x = 2, .y = 1}}),
        atlas);

    EXPECT_EQ(
        renderer.blitsOf(roadTile(antwika::game::kLinkMask)), 1U);
}

TEST_F(GridSceneTest, Draw_ChoosesAWalkerTileByWhichWayItFaces)
{
    constexpr Cell where{.x = 1, .y = 1};

    for (const auto facing : {
             Direction::North,
             Direction::East,
             Direction::South,
             Direction::West,
         })
    {
        RecordingRenderer each;

        scene.draw(
            each,
            kCanvas,
            snapshot(
                Camera(Point{.x = 300, .y = 40}, 2),
                GridExtent{},
                {},
                {WalkerView{.at = where, .facing = facing}}),
            atlas);

        ASSERT_EQ(each.blits.size(), 1U);
        EXPECT_EQ(each.blits.front().source, walkerTile(facing));
    }
}

// A walker is never hidden by the road it is standing on.
TEST_F(GridSceneTest, Draw_BlitsAWalkerAfterTheGroundAndTheRoad)
{
    constexpr Cell where{.x = 0, .y = 0};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 1, .height = 1},
            {where},
            {WalkerView{.at = where, .facing = Direction::North}}),
        atlas);

    ASSERT_EQ(renderer.blits.size(), 3U);
    EXPECT_EQ(renderer.blits[0].source, groundTile());
    EXPECT_EQ(renderer.blits[1].source, roadTile(0));
    EXPECT_EQ(
        renderer.blits[2].source, walkerTile(Direction::North));
}

// The culling claim, asserted rather than assumed.
TEST_F(GridSceneTest, Draw_SkipsEverythingEntirelyOffTheCanvas)
{
    // Panned far away, so nothing reaches the canvas at all.
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = -100000, .y = -100000}, 2),
            GridExtent{.width = 4, .height = 4},
            {Cell{.x = 1, .y = 1}},
            {WalkerView{.at = Cell{.x = 2, .y = 2}}}),
        atlas);

    EXPECT_TRUE(renderer.blits.empty());
}

// Culling has to reject a cell past every edge, not only the near ones.
TEST_F(GridSceneTest, Draw_SkipsCellsPastEachEdgeOfTheCanvas)
{
    for (const auto pan : {
             Point{.x = -100000, .y = 0},
             Point{.x = 0, .y = -100000},
             Point{.x = 100000, .y = 0},
             Point{.x = 0, .y = 100000},
         })
    {
        RecordingRenderer each;

        scene.draw(
            each,
            kCanvas,
            snapshot(
                Camera(pan, 2),
                GridExtent{.width = 2, .height = 2},
                {Cell{.x = 0, .y = 0}},
                {WalkerView{.at = Cell{.x = 1, .y = 1}}}),
            atlas);

        EXPECT_TRUE(each.blits.empty())
            << "pan " << pan.x << "," << pan.y;
    }
}

TEST_F(GridSceneTest, Draw_StillClearsWhenEverythingIsCulled)
{
    MockRenderer strict;

    EXPECT_CALL(strict, clear(_));
    EXPECT_CALL(strict, drawTexture(_, _, _, _)).Times(0);

    scene.draw(
        strict,
        kCanvas,
        snapshot(
            Camera(Point{.x = -100000, .y = -100000}, 2),
            GridExtent{.width = 4, .height = 4}),
        atlas);
}

TEST_F(GridSceneTest, Draw_HandlesAnExtentWithNoCells)
{
    scene.draw(renderer, kCanvas, snapshot(Camera(), GridExtent{}), atlas);

    EXPECT_TRUE(renderer.blits.empty());
}

// A canvas smaller than a tile is the underflow trap blog/012 found.
TEST_F(GridSceneTest, Draw_SurvivesACanvasSmallerThanOneTile)
{
    scene.draw(
        renderer,
        Size{.width = 1, .height = 1},
        snapshot(
            Camera(Point{}, 4),
            GridExtent{.width = 2, .height = 2},
            {Cell{.x = 0, .y = 0}}),
        atlas);

    // Nothing ran off into a four-billion-pixel destination.
    for (const auto &blit : renderer.blits)
    {
        EXPECT_LT(blit.destination.size.width, 1000U);
        EXPECT_LT(blit.destination.size.height, 1000U);
    }
}

// Every zoom level samples the same art, since the atlas holds one size.
TEST_F(GridSceneTest, Draw_SamplesTheSameSourceAtEveryZoom)
{
    for (std::size_t zoom = 0; zoom < antwika::game::kZoomHalfWidths.size();
         ++zoom)
    {
        RecordingRenderer each;
        const Camera camera(Point{.x = 300, .y = 200}, zoom);

        scene.draw(
            each,
            kCanvas,
            snapshot(camera, GridExtent{.width = 1, .height = 1}),
            atlas);

        ASSERT_EQ(each.blits.size(), 1U);
        EXPECT_EQ(each.blits.front().source, groundTile());
        EXPECT_EQ(
            each.blits.front().destination,
            cellBounds(Cell{.x = 0, .y = 0}, camera));
    }
}

TEST_F(GridSceneTest, Draw_PresentsNothingItself)
{
    MockRenderer strict;

    // Presenting belongs to whatever owns the frame, not to the scene.
    EXPECT_CALL(strict, present()).Times(0);
    EXPECT_CALL(strict, clear(_)).Times(AnyNumber());
    EXPECT_CALL(strict, drawTexture(_, _, _, _)).Times(AnyNumber());

    scene.draw(
        strict,
        kCanvas,
        snapshot(Camera(), GridExtent{.width = 1, .height = 1}),
        atlas);
}

TEST_F(GridSceneTest, Draw_AsksNothingOfTheTextureItBlits)
{
    // The atlas layout is fixed, so the scene never has to measure it.
    EXPECT_CALL(atlas, size()).Times(0);

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(),
            GridExtent{.width = 1, .height = 1},
            {Cell{.x = 0, .y = 0}},
            {WalkerView{}}),
        atlas);
}

TEST_F(GridSceneTest, Draw_LaysOneBlitPerBuildingInItsOwnCellsBounds)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            camera,
            GridExtent{},
            {},
            {},
            {BuildingView{.at = where, .kind = BuildingKind::House}}),
        atlas);

    ASSERT_EQ(renderer.blits.size(), 1U);
    EXPECT_EQ(renderer.blits.front().source, buildingTile(BuildingKind::House));
    EXPECT_EQ(renderer.blits.front().destination, cellBounds(where, camera));
    EXPECT_EQ(renderer.blits.front().texture, &atlas);
    EXPECT_EQ(renderer.blits.front().tint, kUntinted);
}

TEST_F(GridSceneTest, Draw_ChoosesABuildingTileByWhatTheBuildingIsFor)
{
    std::vector<Rect> sources;

    for (const auto kind : kEveryBuildingKind)
    {
        RecordingRenderer each;

        scene.draw(
            each,
            kCanvas,
            snapshot(
                Camera(Point{.x = 300, .y = 40}, 2),
                GridExtent{},
                {},
                {},
                {BuildingView{.at = Cell{.x = 1, .y = 1}, .kind = kind}}),
            atlas);

        ASSERT_EQ(each.blits.size(), 1U);
        EXPECT_EQ(each.blits.front().source, buildingTile(kind));
        sources.push_back(each.blits.front().source);
    }

    // No two kinds may share a slot, or two buildings would look alike.
    for (std::size_t i = 0; i < sources.size(); ++i)
    {
        for (std::size_t j = i + 1; j < sources.size(); ++j)
        {
            EXPECT_NE(sources[i], sources[j]) << i << " vs " << j;
        }
    }
}

// The order the class comment argues for, asserted rather than assumed.
TEST_F(GridSceneTest, Draw_BlitsABuildingAfterTheRoadAndBeforeTheWalker)
{
    constexpr Cell where{.x = 0, .y = 0};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 1, .height = 1},
            {where},
            {WalkerView{.at = where, .facing = Direction::North}},
            {BuildingView{.at = where, .kind = BuildingKind::House}}),
        atlas);

    ASSERT_EQ(renderer.blits.size(), 4U);
    EXPECT_EQ(renderer.blits[0].source, groundTile());
    EXPECT_EQ(renderer.blits[1].source, roadTile(0));
    EXPECT_EQ(renderer.blits[2].source, buildingTile(BuildingKind::House));
    EXPECT_EQ(renderer.blits[3].source, walkerTile(Direction::North));
}

// Ascending Cell order is back to front for any two neighbours.
// So the snapshot's own order is the order they are blitted in.
TEST_F(GridSceneTest, Draw_BlitsBuildingsInTheOrderTheyArriveIn)
{
    const std::vector<BuildingView> buildings{
        BuildingView{.at = Cell{.x = 0, .y = 0}, .kind = BuildingKind::House},
        BuildingView{
            .at = Cell{.x = 0, .y = 1}, .kind = BuildingKind::FoodSource},
        BuildingView{
            .at = Cell{.x = 1, .y = 0}, .kind = BuildingKind::WaterSource}};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{},
            {},
            {},
            buildings),
        atlas);

    ASSERT_EQ(renderer.blits.size(), buildings.size());

    for (std::size_t i = 0; i < buildings.size(); ++i)
    {
        EXPECT_EQ(renderer.blits[i].source, buildingTile(buildings[i].kind))
            << "building " << i;
    }
}

TEST_F(GridSceneTest, Draw_TintsAWalkerByWhatItIsCarrying)
{
    std::vector<Color> tints;

    for (const auto kind : kEveryWalkerKind)
    {
        RecordingRenderer each;

        scene.draw(
            each,
            kCanvas,
            snapshot(
                Camera(Point{.x = 300, .y = 40}, 2),
                GridExtent{},
                {},
                {WalkerView{.at = Cell{.x = 1, .y = 1}, .kind = kind}}),
            atlas);

        ASSERT_EQ(each.blits.size(), 1U);
        EXPECT_EQ(each.blits.front().tint, walkerTint(kind));
        tints.push_back(each.blits.front().tint);
    }

    // Telling the four apart is the whole point of the tint.
    for (std::size_t i = 0; i < tints.size(); ++i)
    {
        for (std::size_t j = i + 1; j < tints.size(); ++j)
        {
            EXPECT_NE(tints[i], tints[j]) << i << " vs " << j;
        }
    }
}

TEST_F(GridSceneTest, WalkerTint_LeavesEveryKindBrightEnoughToSee)
{
    // A tint is multiplied into the art rather than replacing it.
    // A dark one would darken a walker rather than name what it holds.
    for (const auto kind : kEveryWalkerKind)
    {
        const auto tint = walkerTint(kind);

        EXPECT_EQ(tint.alpha, 255);
        EXPECT_GE(
            std::max({tint.red, tint.green, tint.blue}),
            static_cast<std::uint8_t>(200));
    }
}

TEST_F(GridSceneTest, StockBarBounds_StandsOnTheCellsTopCorner)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    // Worked by hand, at half-width 16 and a top corner at (300, 56).
    // That makes a 4x16 bar centred on the corner and rising from it.
    EXPECT_EQ(
        stockBarBounds(where, camera),
        (Rect{
            .origin = {.x = 298, .y = 40},
            .size = {.width = 4, .height = 16}}));

    const auto top = cellToScreen(where, camera);
    const auto bar = stockBarBounds(where, camera);

    EXPECT_EQ(bar.origin.y + static_cast<std::int32_t>(bar.size.height), top.y);
}

TEST_F(GridSceneTest, StockFillBounds_GrowsFromTheBottomOfTheBar)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    const BuildingView quarter{.at = where, .held = 25, .capacity = 100};

    EXPECT_EQ(
        stockFillBounds(quarter, camera),
        (Rect{
            .origin = {.x = 298, .y = 52},
            .size = {.width = 4, .height = 4}}));
}

TEST_F(GridSceneTest, StockFillBounds_FillsTheWholeBarAtCapacity)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    const BuildingView full{.at = where, .held = 100, .capacity = 100};

    EXPECT_EQ(stockFillBounds(full, camera), stockBarBounds(where, camera));
}

// Stock above capacity fills the bar rather than overflowing it.
TEST_F(GridSceneTest, StockFillBounds_ClampsStockAboveCapacity)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    const BuildingView over{.at = where, .held = 9999, .capacity = 100};

    EXPECT_EQ(stockFillBounds(over, camera), stockBarBounds(where, camera));
}

TEST_F(GridSceneTest, StockFillBounds_IsEmptyForNothingHeld)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};
    const auto bar = stockBarBounds(where, camera);

    for (const auto held : {0, -5})
    {
        const BuildingView empty{
            .at = where, .held = held, .capacity = 100};
        const auto fill = stockFillBounds(empty, camera);

        EXPECT_EQ(fill.size.height, 0U) << "held " << held;
        EXPECT_EQ(
            fill.origin.y,
            bar.origin.y + static_cast<std::int32_t>(bar.size.height));
    }
}

// The division is what a zero capacity would trap on.
TEST_F(GridSceneTest, StockFillBounds_IsEmptyForABuildingWithNoCapacity)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    for (const auto capacity : {0, -1})
    {
        const BuildingView broken{
            .at = where, .held = 50, .capacity = capacity};

        EXPECT_EQ(stockFillBounds(broken, camera).size.height, 0U)
            << "capacity " << capacity;
    }
}

TEST_F(GridSceneTest, Draw_DrawsABackgroundAndAFillForEachStockBar)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    const BuildingView building{
        .at = Cell{.x = 1, .y = 1}, .held = 50, .capacity = 100};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(camera, GridExtent{}, {}, {}, {building}),
        atlas);

    ASSERT_EQ(renderer.fills.size(), 2U);
    EXPECT_EQ(renderer.fills[0].rect, stockBarBounds(building.at, camera));
    EXPECT_EQ(renderer.fills[1].rect, stockFillBounds(building, camera));
    EXPECT_NE(renderer.fills[0].color, renderer.fills[1].color);
}

TEST_F(GridSceneTest, Draw_DrawsOnlyTheBackgroundForAnEmptyStock)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);

    for (const auto building : {
             BuildingView{.at = Cell{.x = 1, .y = 1}, .held = 0},
             BuildingView{
                 .at = Cell{.x = 1, .y = 1}, .held = 50, .capacity = 0},
         })
    {
        RecordingRenderer each;

        scene.draw(
            each,
            kCanvas,
            snapshot(camera, GridExtent{}, {}, {}, {building}),
            atlas);

        ASSERT_EQ(each.fills.size(), 1U);
        EXPECT_EQ(each.fills.front().rect, stockBarBounds(building.at, camera));
    }
}

// A bar goes through the same projection the tiles do.
// So panning moves it exactly as far as it moves the building.
TEST_F(GridSceneTest, Draw_MovesAStockBarWithThePan)
{
    constexpr Cell where{.x = 1, .y = 1};
    const BuildingView building{.at = where, .held = 50, .capacity = 100};

    RecordingRenderer before;
    RecordingRenderer after;
    const Camera first(Point{.x = 300, .y = 200}, 2);
    const Camera second(Point{.x = 317, .y = 209}, 2);

    scene.draw(
        before,
        kCanvas,
        snapshot(first, GridExtent{}, {}, {}, {building}),
        atlas);
    scene.draw(
        after,
        kCanvas,
        snapshot(second, GridExtent{}, {}, {}, {building}),
        atlas);

    ASSERT_EQ(before.fills.size(), 2U);
    ASSERT_EQ(after.fills.size(), 2U);

    for (std::size_t i = 0; i < 2; ++i)
    {
        EXPECT_EQ(after.fills[i].rect.origin.x - before.fills[i].rect.origin.x,
                  17);
        EXPECT_EQ(after.fills[i].rect.origin.y - before.fills[i].rect.origin.y,
                  9);
        EXPECT_EQ(after.fills[i].rect.size, before.fills[i].rect.size);
    }
}

TEST_F(GridSceneTest, Draw_SizesAStockBarFromTheZoomAtEveryLevel)
{
    constexpr Cell where{.x = 0, .y = 0};
    const BuildingView building{.at = where, .held = 50, .capacity = 100};

    std::uint32_t taller = 0;

    for (std::size_t zoom = 0; zoom < antwika::game::kZoomHalfWidths.size();
         ++zoom)
    {
        RecordingRenderer each;
        const Camera camera(Point{.x = 300, .y = 200}, zoom);

        scene.draw(
            each,
            kCanvas,
            snapshot(camera, GridExtent{}, {}, {}, {building}),
            atlas);

        ASSERT_EQ(each.fills.size(), 2U) << "zoom " << zoom;
        EXPECT_EQ(each.fills[0].rect, stockBarBounds(where, camera));
        EXPECT_EQ(each.fills[1].rect, stockFillBounds(building, camera));

        // Never zero, or a bar would be invisible at the furthest zoom.
        EXPECT_GT(each.fills[0].rect.size.width, 0U) << "zoom " << zoom;
        EXPECT_GT(each.fills[0].rect.size.height, taller) << "zoom " << zoom;

        taller = each.fills[0].rect.size.height;
    }
}

TEST_F(GridSceneTest, Draw_SkipsABuildingAndItsBarOffTheCanvas)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = -100000, .y = -100000}, 2),
            GridExtent{},
            {},
            {},
            {BuildingView{.at = Cell{.x = 3, .y = 3}, .held = 50}}),
        atlas);

    EXPECT_TRUE(renderer.blits.empty());
    EXPECT_TRUE(renderer.fills.empty());
}

// A bar is a readout, so nothing on the grid may cover one.
TEST_F(GridSceneTest, Draw_DrawsEveryStockBarAfterEveryBlit)
{
    MockRenderer strict;
    const InSequence order;

    EXPECT_CALL(strict, clear(_));
    EXPECT_CALL(strict, drawTexture(_, _, _, _)).Times(3);
    EXPECT_CALL(strict, drawRect(_, _)).Times(2);

    scene.draw(
        strict,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 1, .height = 1},
            {},
            {WalkerView{.at = Cell{.x = 0, .y = 0}}},
            {BuildingView{
                .at = Cell{.x = 0, .y = 0}, .held = 50, .capacity = 100}}),
        atlas);
}

TEST_F(GridSceneTest, Draw_DrawsNoRectangleWhenThereAreNoBuildings)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 1, .height = 1},
            {Cell{.x = 0, .y = 0}},
            {WalkerView{.at = Cell{.x = 0, .y = 0}}}),
        atlas);

    EXPECT_TRUE(renderer.fills.empty());
}

// The two views the scene draws from, compared field by field.
// These are the fields this file's tests vary.
// A defaulted operator== ignoring one would weaken every one of them.
TEST_F(GridSceneTest, WalkerView_ComparesEveryFieldIndependently)
{
    constexpr WalkerView same{};

    EXPECT_EQ(same, (WalkerView{}));
    EXPECT_NE(same, (WalkerView{.at = Cell{.x = 1, .y = 0}}));
    EXPECT_NE(same, (WalkerView{.at = {}, .facing = Direction::West}));
    EXPECT_NE(same, (WalkerView{.at = {}, .kind = WalkerKind::Fireman}));
    EXPECT_NE(same, (WalkerView{.at = {}, .carried = 3}));
}

TEST_F(GridSceneTest, BuildingView_ComparesEveryFieldIndependently)
{
    constexpr BuildingView same{};

    EXPECT_EQ(same, (BuildingView{}));
    EXPECT_NE(same, (BuildingView{.at = Cell{.x = 1, .y = 0}}));
    EXPECT_NE(
        same, (BuildingView{.at = {}, .kind = BuildingKind::FireStation}));
    EXPECT_NE(same, (BuildingView{.at = {}, .held = 7}));
    EXPECT_NE(same, (BuildingView{.at = {}, .capacity = 7}));
}

TEST_F(GridSceneTest, SceneSnapshot_ComparesTheBuildingsToo)
{
    const auto empty = snapshot(Camera(), GridExtent{});

    EXPECT_EQ(empty, snapshot(Camera(), GridExtent{}));
    EXPECT_NE(
        empty,
        snapshot(Camera(), GridExtent{}, {}, {}, {BuildingView{}}));
}
