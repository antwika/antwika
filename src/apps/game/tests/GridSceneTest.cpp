#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
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
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/ResourceBar.hpp"
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

    // Which kind of call happened, in the order the calls happened.
    // Kept beside the recorded values rather than inside them.
    // The frame-to-frame comparisons stay comparisons of pictures.
    enum class Call
    {
        Blit,
        Rect,
        Text,
    };

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
                        order.push_back(Call::Blit);
                    });

            ON_CALL(*this, drawRect(_, _))
                .WillByDefault(
                    [this](Rect rect, Color color)
                    {
                        rects.push_back(Filled{rect, color});
                        order.push_back(Call::Rect);
                    });

            ON_CALL(*this, drawText(_, _, _, _))
                .WillByDefault(
                    [this](
                        antwika::gfx::Point origin,
                        std::string_view text,
                        std::uint32_t scale,
                        Color color)
                    {
                        texts.push_back(
                            Written{
                                origin, std::string(text), scale, color});
                        order.push_back(Call::Text);
                    });
        }

        struct Filled
        {
            Rect rect;
            Color color;

            [[nodiscard]] bool operator==(const Filled &other) const
                = default;
        };

        struct Written
        {
            antwika::gfx::Point origin;
            std::string text;
            std::uint32_t scale = 0;
            Color color;

            [[nodiscard]] bool operator==(const Written &other) const
                = default;
        };

        std::vector<Filled> rects;
        std::vector<Written> texts;
        std::vector<Call> order;

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
            .ghost = {},
            .hover = {}};
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

// The gauges: a small vertical bar per resource a building depends on.
// Drawn as rectangles rather than blitted.
// A fraction of a capacity is not art -- see ResourceBar.hpp.
TEST_F(GridSceneTest, Draw_GaugesEachBuildingThatDependsOnSomething)
{
    const Camera camera(Point{.x = 300, .y = 40}, 3);
    const antwika::game::BuildingSprite house{
        .at = Cell{.x = 0, .y = 0},
        .kind = antwika::game::BuildingKind::House,
        .stock = {50, 50}};

    auto scene_ = snapshot(camera, GridExtent{});
    scene_.buildings.push_back(house);

    scene.draw(renderer, kCanvas, scene_, atlas);

    const auto bars = antwika::game::buildingBars(house, camera);

    ASSERT_EQ(bars.size(), 2U);

    // One track and one fill per bar, and each is the bar's own value.
    ASSERT_EQ(renderer.rects.size(), 4U);
    EXPECT_EQ(renderer.rects[0].rect, bars[0].track);
    EXPECT_EQ(renderer.rects[0].color, antwika::game::kBarTrack);
    EXPECT_EQ(renderer.rects[1].rect, bars[0].fill);
    EXPECT_EQ(
        renderer.rects[1].color,
        antwika::game::resourceColour(bars[0].resource));
}

// A bar with nothing in it is a track and no fill at all.
TEST_F(GridSceneTest, Draw_DrawsNoFillForAnEmptyGauge)
{
    auto scene_ = snapshot(Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 0, .y = 0},
            .kind = antwika::game::BuildingKind::House,
            .stock = {0, 0}});

    scene.draw(renderer, kCanvas, scene_, atlas);

    EXPECT_EQ(renderer.rects.size(), 2U);
}

// A source depends on nothing, so it is gauged for nothing.
TEST_F(GridSceneTest, Draw_GaugesNeitherASourceNorABuildingOffTheCanvas)
{
    auto scene_ = snapshot(Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 0, .y = 0},
            .kind = antwika::game::BuildingKind::FoodSource,
            .stock = {50, 50}});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 900, .y = -900},
            .kind = antwika::game::BuildingKind::House,
            .stock = {50, 50}});

    scene.draw(renderer, kCanvas, scene_, atlas);

    EXPECT_TRUE(renderer.rects.empty());
}

TEST_F(GridSceneTest, Draw_GaugesAWalkerWithWhatItIsCarrying)
{
    const Camera camera(Point{.x = 300, .y = 40}, 3);
    const WalkerSprite walker{
        .at = Cell{.x = 1, .y = 1},
        .kind = antwika::game::WalkerKind::Food,
        .carried = antwika::game::kWalkerLoad};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(camera, GridExtent{}, {}, {walker}),
        atlas);

    const auto bars =
        antwika::game::walkerBars(walker, camera, Progress());

    ASSERT_EQ(bars.size(), 1U);
    ASSERT_EQ(renderer.rects.size(), 2U);
    EXPECT_EQ(renderer.rects[0].rect, bars[0].track);
    EXPECT_EQ(renderer.rects[1].rect, bars[0].fill);
}

TEST_F(GridSceneTest, Draw_GaugesNoWalkerThatIsOffTheCanvasOrCarriesNone)
{
    const Camera camera(Point{.x = -100000, .y = -100000}, 3);

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            camera,
            GridExtent{},
            {},
            {WalkerSprite{
                .at = Cell{.x = 1, .y = 1},
                .kind = antwika::game::WalkerKind::Food,
                .carried = 50}}),
        atlas);

    EXPECT_TRUE(renderer.rects.empty());

    RecordingRenderer nearby;
    scene.draw(
        nearby,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 3),
            GridExtent{},
            {},
            {WalkerSprite{
                .at = Cell{.x = 1, .y = 1},
                .kind = antwika::game::WalkerKind::Fireman}}),
        atlas);

    EXPECT_TRUE(nearby.rects.empty());
}

// A gauge is drawn after every sprite.
// So nothing standing in front of what it gauges can hide it.
TEST_F(GridSceneTest, Draw_DrawsEveryGaugeAfterEverySprite)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2},
        {Cell{.x = 0, .y = 1}},
        {WalkerSprite{
            .at = Cell{.x = 0, .y = 1},
            .kind = antwika::game::WalkerKind::Water,
            .carried = 40}});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 1, .y = 1},
            .kind = antwika::game::BuildingKind::House,
            .stock = {50, 50}});

    scene.draw(renderer, kCanvas, scene_, atlas);

    ASSERT_FALSE(renderer.order.empty());

    const auto firstRect =
        std::find(renderer.order.begin(), renderer.order.end(), Call::Rect);
    const auto lastBlit = std::find(
        renderer.order.rbegin(), renderer.order.rend(), Call::Blit);

    ASSERT_NE(firstRect, renderer.order.end());
    ASSERT_NE(lastBlit, renderer.order.rend());
    EXPECT_GT(
        static_cast<std::size_t>(firstRect - renderer.order.begin()),
        renderer.order.size() - 1
            - static_cast<std::size_t>(
                lastBlit - renderer.order.rbegin()));
}

// The hover readout: drawn from the same snapshot, last of everything.
TEST_F(GridSceneTest, Draw_SaysNothingWithNothingUnderThePointer)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(Camera(Point{.x = 300, .y = 40}, 3), GridExtent{}),
        atlas);

    EXPECT_TRUE(renderer.texts.empty());
}

TEST_F(GridSceneTest, Draw_WritesTheHoverPanelLastAndWhereItWasLaidOut)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2});
    scene_.hover = antwika::game::HoverReadout{
        .anchor = Point{.x = 40, .y = 50},
        .building = antwika::game::BuildingSprite{
            .at = Cell{.x = 0, .y = 0},
            .kind = antwika::game::BuildingKind::House,
            .stock = {30, 70}}};

    scene.draw(renderer, kCanvas, scene_, atlas);

    const auto panel = antwika::game::readoutPanel(scene_.hover, kCanvas);

    ASSERT_EQ(panel.lines.size(), 3U);
    ASSERT_EQ(renderer.texts.size(), 3U);

    for (std::size_t line = 0; line < panel.lines.size(); ++line)
    {
        EXPECT_EQ(renderer.texts[line].origin, panel.lines[line].origin);
        EXPECT_EQ(renderer.texts[line].text, panel.lines[line].text);
        EXPECT_EQ(renderer.texts[line].color, panel.lines[line].colour);
        EXPECT_EQ(
            renderer.texts[line].scale,
            antwika::game::kReadoutTextScale);
    }

    // Its backdrop is the last rectangle, over the gauges and the grid.
    ASSERT_FALSE(renderer.rects.empty());
    EXPECT_EQ(renderer.rects.back().rect, panel.box);
    EXPECT_EQ(renderer.rects.back().color, antwika::game::kReadoutBackdrop);
    EXPECT_EQ(renderer.order.back(), Call::Text);
}

// The panel is about the picture and never about the state.
// So the same snapshot without a hover draws the same grid.
TEST_F(GridSceneTest, Draw_LeavesTheGridAloneWhetherOrNotAnythingHovers)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2},
        {Cell{.x = 0, .y = 0}});

    scene.draw(renderer, kCanvas, scene_, atlas);
    const auto blind = renderer.blits;

    scene_.hover = antwika::game::HoverReadout{
        .anchor = Point{.x = 40, .y = 50},
        .walker = WalkerSprite{}};

    RecordingRenderer watched;
    scene.draw(watched, kCanvas, scene_, atlas);

    EXPECT_EQ(watched.blits, blind);
    EXPECT_FALSE(watched.texts.empty());
}
