#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/animation/Progress.hpp>
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

using antwika::animation::Progress;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::Direction;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::groundTile;
using antwika::game::linkBit;
using antwika::game::roadTile;
using antwika::game::SceneSnapshot;
using antwika::game::walkerTile;
using antwika::game::WalkerSprite;
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
        }

        struct Blit
        {
            const ITexture *texture;
            Rect source;
            Rect destination;
            Color tint;

            [[nodiscard]] bool operator==(const Blit &other) const = default;
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
    };

    [[nodiscard]] SceneSnapshot snapshot(
        Camera camera,
        GridExtent extent,
        std::vector<Cell> paths = {},
        std::vector<WalkerSprite> walkers = {})
    {
        return SceneSnapshot{
            .camera = camera,
            .extent = extent,
            .paths = std::move(paths),
            .walkers = std::move(walkers),
            .buildings = {},
            .ghost = {}};
    }
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
            {WalkerSprite{.at = where, .facing = Direction::East}}),
        atlas);

    ASSERT_EQ(renderer.blits.size(), 2U);

    for (const auto &blit : renderer.blits)
    {
        EXPECT_EQ(blit.destination, cellBounds(where, camera));
    }
}

// The art carries every colour the grid has.
// A tint would mean the picture was decided in two places.
TEST_F(GridSceneTest, Draw_BlitsTheAtlasItIsGivenAndTintsNothing)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 2, .height = 2},
            {Cell{.x = 0, .y = 0}},
            {WalkerSprite{.at = Cell{.x = 0, .y = 0}}}),
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
                {WalkerSprite{.at = where, .facing = facing}}),
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
            {WalkerSprite{.at = where, .facing = Direction::North}}),
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
            {WalkerSprite{.at = Cell{.x = 2, .y = 2}}}),
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
                {WalkerSprite{.at = Cell{.x = 1, .y = 1}}}),
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
            {WalkerSprite{}}),
        atlas);
}

TEST_F(GridSceneTest, Draw_SlidesAWalkerBetweenTheCellsItIsStepping)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell from{.x = 1, .y = 1};
    constexpr Cell to{.x = 2, .y = 1};

    const auto stepping = snapshot(
        camera,
        GridExtent{},
        {},
        {WalkerSprite{
            .at = to,
            .facing = Direction::East,
            .from = from,
            .ticksIntoStep = 0}});

    scene.draw(renderer, kCanvas, stepping, atlas, Progress());
    ASSERT_EQ(renderer.blits.size(), 1U);
    const auto start = renderer.blits[0].destination;

    renderer.blits.clear();
    scene.draw(renderer, kCanvas, stepping, atlas, Progress(1, 2));
    ASSERT_EQ(renderer.blits.size(), 1U);
    const auto middle = renderer.blits[0].destination;

    // The first frame of a step is still on the cell it left.
    // A later frame of the same tick has moved on.
    // And the snapshot did not change between the two.
    EXPECT_EQ(start, cellBounds(from, camera));
    EXPECT_NE(middle, start);
    EXPECT_NE(middle, cellBounds(to, camera));
}

TEST_F(GridSceneTest, Draw_LeavesEverythingButTheWalkersWhereItWas)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    const auto scene_ = snapshot(camera, GridExtent{.width = 2, .height = 2},
        {where}, {});

    scene.draw(renderer, kCanvas, scene_, atlas, Progress());
    const auto atTick = renderer.blits;

    renderer.blits.clear();
    scene.draw(renderer, kCanvas, scene_, atlas, Progress(1, 2));

    // A cell does not move between two ticks.
    // So only the walkers differ from one frame to the next.
    EXPECT_EQ(renderer.blits, atTick);
}
