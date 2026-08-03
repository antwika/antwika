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

#include "TestTranslator.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/ResourceColour.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::game::tests::kTranslator;

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
            .plan = {},
            .ghost = {},
            .hover = {},
            .overlay = {}};
    }
} // namespace

class GridSceneTest : public ::testing::Test
{
protected:
    RecordingRenderer renderer;

    // Three sheets, distinct, so a blit from the wrong one is caught.
    NiceMock<MockTexture> atlas;
    NiceMock<MockTexture> atlas2x2;
    NiceMock<MockTexture> atlas3x3;
    antwika::game::AtlasTextures atlases{
        .oneByOne = atlas,
        .twoByTwo = atlas2x2,
        .threeByThree = atlas3x3};

    GridScene scene{kTranslator};
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
        atlases);
}

TEST_F(GridSceneTest, Draw_LaysOneGroundTilePerVisibleCell)
{
    constexpr GridExtent extent{.width = 3, .height = 3};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(Camera(Point{.x = 300, .y = 40}, 2), extent),
        atlases);

    EXPECT_EQ(renderer.blitsOf(groundTile()), 3U * 3U);
    EXPECT_EQ(renderer.blits.size(), 3U * 3U);
}

TEST_F(GridSceneTest, Draw_BlitsEachSpriteIntoItsOwnCellsBox)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            camera,
            GridExtent{.width = 2, .height = 2},
            {where},
            {WalkerSprite{.at = where, .facing = Direction::East}}),
        atlases);

    // Four ground sprites, the road over its cell, and the walker.
    ASSERT_EQ(renderer.blits.size(), 6U);

    // The road and the still walker share the cell's own sprite box.
    // The box hangs off the diamond's bottom corner -- SpriteBounds.
    EXPECT_EQ(
        renderer.blits[4].source,
        roadTile(0));
    EXPECT_EQ(
        renderer.blits[4].destination,
        antwika::game::tileSpriteBounds(where, camera));
    EXPECT_EQ(
        renderer.blits[5].source, walkerTile(Direction::East));
    EXPECT_EQ(
        renderer.blits[5].destination,
        antwika::game::tileSpriteBounds(where, camera));
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
        atlases);

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
            GridExtent{.width = 3, .height = 3},
            {Cell{.x = 1, .y = 1}}),
        atlases);

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
            GridExtent{.width = 4, .height = 4},
            {Cell{.x = 0, .y = 1},
             Cell{.x = 1, .y = 1},
             Cell{.x = 2, .y = 1}}),
        atlases);

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
            GridExtent{.width = 4, .height = 4},
            // Ascending, as PathIndex's set hands them over.
            {Cell{.x = 0, .y = 1},
             Cell{.x = 1, .y = 0},
             Cell{.x = 1, .y = 1},
             Cell{.x = 1, .y = 2},
             Cell{.x = 2, .y = 1}}),
        atlases);

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
            atlases);

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
        atlases);

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
        atlases);

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
            atlases);

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
        atlases);
}

TEST_F(GridSceneTest, Draw_HandlesAnExtentWithNoCells)
{
    scene.draw(renderer, kCanvas, snapshot(Camera(), GridExtent{}), atlases);

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
        atlases);

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
            atlases);

        ASSERT_EQ(each.blits.size(), 1U);
        EXPECT_EQ(each.blits.front().source, groundTile());
        EXPECT_EQ(
            each.blits.front().destination,
            antwika::game::tileSpriteBounds(Cell{.x = 0, .y = 0}, camera));
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
        atlases);
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
        atlases);
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

    scene.draw(renderer, kCanvas, stepping, atlases, Progress());
    ASSERT_EQ(renderer.blits.size(), 1U);
    const auto start = renderer.blits[0].destination;

    renderer.blits.clear();
    scene.draw(renderer, kCanvas, stepping, atlases, Progress(1, 2));
    ASSERT_EQ(renderer.blits.size(), 1U);
    const auto middle = renderer.blits[0].destination;

    // The first frame of a step is still on the cell it left.
    // A later frame of the same tick has moved on.
    // And the snapshot did not change between the two.
    EXPECT_EQ(start, antwika::game::tileSpriteBounds(from, camera));
    EXPECT_NE(middle, start);
    EXPECT_NE(middle, antwika::game::tileSpriteBounds(to, camera));
}

// The whole ticks of a step stop when WalkerSystem does.
// The frames drawn between two ticks do not stop with them.
// So a held walker drawn from the sub-tick slides and snaps back.
// Decided here rather than by whoever supplies the fraction.
TEST_F(GridSceneTest, Draw_HoldsAWalkerStillWhilePaused)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell from{.x = 1, .y = 1};
    constexpr Cell to{.x = 2, .y = 1};

    auto held = snapshot(
        camera,
        GridExtent{},
        {},
        {WalkerSprite{
            .at = to,
            .facing = Direction::East,
            .from = from,
            .ticksIntoStep = 1}});
    held.paused = true;

    scene.draw(renderer, kCanvas, held, atlases, Progress());
    ASSERT_EQ(renderer.blits.size(), 1U);
    const auto atTick = renderer.blits[0].destination;

    renderer.blits.clear();
    scene.draw(renderer, kCanvas, held, atlases, Progress(1, 2));
    ASSERT_EQ(renderer.blits.size(), 1U);

    EXPECT_EQ(renderer.blits[0].destination, atTick);

    // Held where its step had got to, rather than on either cell.
    // A pause stops a walker; it does not tidy it onto a tile.
    EXPECT_NE(atTick, antwika::game::tileSpriteBounds(from, camera));
    EXPECT_NE(atTick, antwika::game::tileSpriteBounds(to, camera));
}

// The gauge is drawn from the box the sprite is blitted into.
// So a held bar has to be held with it, or the two part company.
TEST_F(GridSceneTest, Draw_LeavesEverythingButTheWalkersWhereItWas)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    const auto scene_ = snapshot(camera, GridExtent{.width = 2, .height = 2},
        {where}, {});

    scene.draw(renderer, kCanvas, scene_, atlases, Progress());
    const auto atTick = renderer.blits;

    renderer.blits.clear();
    scene.draw(renderer, kCanvas, scene_, atlases, Progress(1, 2));

    // A cell does not move between two ticks.
    // So only the walkers differ from one frame to the next.
    EXPECT_EQ(renderer.blits, atTick);
}

// Nothing is gauged over the art any more.
// What a building holds is read off the hover panel instead.
// A row of bars over every sprite made a district unreadable.
// So the only rectangles a grid draws are that panel's own.
TEST_F(GridSceneTest, Draw_PaintsNoGaugeOverABuildingOrAWalker)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2},
        {Cell{.x = 0, .y = 1}},
        {WalkerSprite{
            .at = Cell{.x = 0, .y = 1},
            .kind = antwika::game::WalkerKind::MarketSeller,
            .carried = antwika::game::kWalkerLoad}});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 1, .y = 1},
            .kind = antwika::game::BuildingKind::House,
            .stock = {50, 50, 50}});

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_TRUE(renderer.rects.empty());
}

// An overlay is the ground sprite tinted, cell by cell.
// A cell is a diamond and drawRect() takes an upright box.
// Which is the same reason the ghost's edge is four lines.
TEST_F(GridSceneTest, Draw_PaintsNoOverlayForTheCityItself)
{
    const auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2});

    scene.draw(renderer, kCanvas, scene_, atlases);

    // Four ground sprites, and no second pass over any of them.
    EXPECT_EQ(renderer.blitsOf(groundTile()), 4U);
}

TEST_F(GridSceneTest, Draw_ScrimsEveryCellAndTintsTheOnesWithAValue)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2});
    scene_.view = antwika::game::MapView::Desirability;
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    std::size_t scrimmed = 0;
    std::size_t tinted = 0;

    for (const auto &blit : renderer.blits)
    {
        if (blit.tint == antwika::game::kOverlayScrim)
        {
            ++scrimmed;
        }

        if (blit.tint
            == antwika::game::overlayColour(
                antwika::game::MapView::Desirability))
        {
            ++tinted;
        }
    }

    // The scrim over all four, and the value over the one cell.
    // A district nothing reaches is what somebody looks for here.
    // So it has to be visibly darker rather than simply unpainted.
    EXPECT_EQ(scrimmed, 4U);
    EXPECT_EQ(tinted, 1U);
}

// Under the walkers.
// A walker is a thing in the city rather than a fact about it.
TEST_F(GridSceneTest, Draw_PaintsTheOverlayBeforeTheWalkers)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2},
        {},
        {WalkerSprite{.at = Cell{.x = 1, .y = 1}}});
    scene_.view = antwika::game::MapView::Water;
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    std::size_t lastScrim = 0;
    std::size_t walker = 0;

    for (std::size_t index = 0; index < renderer.blits.size(); ++index)
    {
        if (renderer.blits[index].tint == antwika::game::kOverlayScrim)
        {
            lastScrim = index;
        }

        if (renderer.blits[index].source
            == walkerTile(Direction::East))
        {
            walker = index;
        }
    }

    EXPECT_GT(walker, lastScrim);
}

// The hover readout: drawn from the same snapshot, last of everything.
TEST_F(GridSceneTest, Draw_SaysNothingWithNothingUnderThePointer)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(Camera(Point{.x = 300, .y = 40}, 3), GridExtent{}),
        atlases);

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
            .stock = {30, 70, 10}}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto panel = antwika::game::readoutPanel(
        scene_.hover, kCanvas, kTranslator);

    // Its name, its tier, how full it is, and one line per resource.
    ASSERT_EQ(panel.lines.size(), 3 + antwika::game::kResourceCount);
    ASSERT_EQ(renderer.texts.size(), panel.lines.size());

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

    scene.draw(renderer, kCanvas, scene_, atlases);
    const auto blind = renderer.blits;

    scene_.hover = antwika::game::HoverReadout{
        .anchor = Point{.x = 40, .y = 50},
        .walker = WalkerSprite{}};

    RecordingRenderer watched;
    scene.draw(watched, kCanvas, scene_, atlases);

    EXPECT_EQ(watched.blits, blind);
    EXPECT_FALSE(watched.texts.empty());
}

// A planned run of road is previewed faintly, cell by cell.
TEST_F(GridSceneTest, Draw_PreviewsThePlannedRunOfRoad)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 4, .height = 4});
    scene_.plan = antwika::game::RoadPlan{
        .cells =
            {Cell{.x = 1, .y = 1},
             Cell{.x = 2, .y = 1},
             Cell{.x = 3, .y = 1}},
        .valid = true};

    scene.draw(renderer, kCanvas, scene_, atlases);

    std::size_t previewed = 0;

    for (const auto &blit : renderer.blits)
    {
        if (blit.tint.alpha == 110 && blit.tint.green == 255)
        {
            ++previewed;
        }
    }

    EXPECT_EQ(previewed, 3U);
}

// A previewed run shows the junctions the *whole run* would make.
// Worked out against the roads and the rest of the plan together.
// Asked of the roads alone, a route over bare ground has no arms.
// So the preview came out as a string of loose stubs.
// Which is not what its release lays.
TEST_F(GridSceneTest, Draw_PreviewsAPlannedRunAsOneConnectedRoad)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 4, .height = 4});
    scene_.plan = antwika::game::RoadPlan{
        .cells =
            {Cell{.x = 1, .y = 1},
             Cell{.x = 2, .y = 1},
             Cell{.x = 3, .y = 1}},
        .valid = true};

    scene.draw(renderer, kCanvas, scene_, atlases);

    std::vector<Rect> previewed;

    for (const auto &blit : renderer.blits)
    {
        if (blit.tint.alpha == 110 && blit.tint.green == 255)
        {
            previewed.push_back(blit.source);
        }
    }

    // West end, straight through, east end -- a road, not three stubs.
    const std::vector<Rect> wanted{
        roadTile(antwika::game::linkBit(Direction::East)),
        roadTile(
            antwika::game::linkBit(Direction::East)
            | antwika::game::linkBit(Direction::West)),
        roadTile(antwika::game::linkBit(Direction::West))};

    EXPECT_EQ(previewed, wanted);
}

// And a run laid up against a road already there joins onto it.
TEST_F(GridSceneTest, Draw_JoinsAPlannedRunOntoTheRoadsAlreadyThere)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 4, .height = 4},
        {Cell{.x = 1, .y = 1}});
    scene_.plan = antwika::game::RoadPlan{
        .cells = {Cell{.x = 2, .y = 1}}, .valid = true};

    scene.draw(renderer, kCanvas, scene_, atlases);

    for (const auto &blit : renderer.blits)
    {
        if (blit.tint.alpha == 110 && blit.tint.green == 255)
        {
            EXPECT_EQ(
                blit.source,
                roadTile(antwika::game::linkBit(Direction::West)));
        }
    }
}

// A refused run is reddened rather than hidden, as a block is.
TEST_F(GridSceneTest, Draw_ReddensARefusedRunOfRoad)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 4, .height = 4});
    scene_.plan = antwika::game::RoadPlan{
        .cells = {Cell{.x = 1, .y = 1}, Cell{.x = 3, .y = 1}},
        .valid = false};

    scene.draw(renderer, kCanvas, scene_, atlases);

    std::size_t refused = 0;

    for (const auto &blit : renderer.blits)
    {
        if (blit.tint.alpha == 110 && blit.tint.green == 90)
        {
            ++refused;
        }
    }

    EXPECT_EQ(refused, 2U);
}

// Culled on where it would be drawn, as everything else here is.
TEST_F(GridSceneTest, Draw_PreviewsNoPlannedCellOffTheCanvas)
{
    auto scene_ = snapshot(
        Camera(Point{.x = -100000, .y = -100000}, 3),
        GridExtent{.width = 4, .height = 4});
    scene_.plan = antwika::game::RoadPlan{
        .cells = {Cell{.x = 1, .y = 1}}, .valid = true};

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_TRUE(renderer.blits.empty());
}

// A building blits from the sheet its footprint names.
// The wrong sheet would draw a farm out of the storehouse's pixels.
TEST_F(GridSceneTest, Draw_BlitsABuildingFromItsFootprintsOwnSheet)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);

    const struct
    {
        antwika::game::BuildingKind kind;
        const ITexture *sheet;
    } cases[] = {
        {antwika::game::BuildingKind::House, &atlas},
        {antwika::game::BuildingKind::Farm, &atlas2x2},
        {antwika::game::BuildingKind::Storage, &atlas3x3},
    };

    for (const auto &expected : cases)
    {
        RecordingRenderer each;
        auto scene_ = snapshot(camera, GridExtent{});
        scene_.buildings.push_back(
            antwika::game::BuildingSprite{
                .at = Cell{.x = 0, .y = 0}, .kind = expected.kind});

        scene.draw(each, kCanvas, scene_, atlases);

        ASSERT_EQ(each.blits.size(), 1U);
        EXPECT_EQ(each.blits.front().texture, expected.sheet);
        EXPECT_EQ(
            each.blits.front().source,
            antwika::game::buildingTile(expected.kind));
        EXPECT_EQ(
            each.blits.front().destination,
            antwika::game::buildingSpriteBounds(
                Cell{.x = 0, .y = 0}, expected.kind, camera));
    }
}

// A building's art owns its whole block, so no ground goes under it.
// Painting some would cost a blit nobody sees.
TEST_F(GridSceneTest, Draw_LaysNoGroundUnderABuildingsBlock)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 2, .height = 2});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 0, .y = 0},
            .kind = antwika::game::BuildingKind::Farm});

    scene.draw(renderer, kCanvas, scene_, atlases);

    // The farm covers the whole extent, so it is the one blit there is.
    ASSERT_EQ(renderer.blits.size(), 1U);
    EXPECT_EQ(
        renderer.blits.front().source,
        antwika::game::buildingTile(antwika::game::BuildingKind::Farm));
}

// A 3x3's west and north flank cells lie one diagonal past its flush.
// Painted there, their skirts stamped over the already-drawn art.
// So their ground and roads go down in the block's own phase instead.
TEST_F(GridSceneTest, Draw_PaintsAWideBlocksFlankBehindItsArt)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 5, .height = 5});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 1, .y = 0},
            .kind = antwika::game::BuildingKind::Storage});

    // A road on the west flank cell, one diagonal past the flush.
    scene_.paths.push_back(Cell{.x = 0, .y = 2});

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto storage = antwika::game::buildingTile(
        antwika::game::BuildingKind::Storage);

    std::size_t storageAt = renderer.blits.size();
    std::size_t roadAt = renderer.blits.size();

    for (std::size_t blit = 0; blit < renderer.blits.size(); ++blit)
    {
        if (renderer.blits[blit].source == storage)
        {
            storageAt = blit;
        }

        if (renderer.blits[blit].source == roadTile(0))
        {
            roadAt = blit;
        }
    }

    ASSERT_LT(storageAt, renderer.blits.size());
    ASSERT_LT(roadAt, renderer.blits.size());
    EXPECT_LT(roadAt, storageAt);

    // Pulled forward, never painted a second time.
    EXPECT_EQ(renderer.blitsOf(roadTile(0)), 1U);
}

// Two blocks corner to corner share one flank cell.
// One's west flank is the other's north; it is pulled exactly once.
TEST_F(GridSceneTest, Draw_PullsASharedFlankCellOnce)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 20}, 1),
        GridExtent{.width = 6, .height = 6});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 3, .y = 0},
            .kind = antwika::game::BuildingKind::Storage});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 0, .y = 3},
            .kind = antwika::game::BuildingKind::Storage});

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto shared = antwika::game::tileSpriteBounds(
        Cell{.x = 2, .y = 2}, scene_.camera);

    std::size_t drawn = 0;
    std::size_t sharedAt = 0;
    std::size_t firstBlock = renderer.blits.size();

    for (std::size_t blit = 0; blit < renderer.blits.size(); ++blit)
    {
        if (renderer.blits[blit].source == groundTile()
            && renderer.blits[blit].destination == shared)
        {
            ++drawn;
            sharedAt = blit;
        }

        if (renderer.blits[blit].source
                == antwika::game::buildingTile(
                    antwika::game::BuildingKind::Storage)
            && blit < firstBlock)
        {
            firstBlock = blit;
        }
    }

    EXPECT_EQ(drawn, 1U);
    EXPECT_LT(sharedAt, firstBlock);
}

// A flank cell another block stands on is that block's art to paint.
TEST_F(GridSceneTest, Draw_LeavesACoveredFlankToItsOwnBlock)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 5, .height = 5});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 1, .y = 0},
            .kind = antwika::game::BuildingKind::Storage});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 0, .y = 2},
            .kind = antwika::game::BuildingKind::House});

    scene.draw(renderer, kCanvas, scene_, atlases);

    // The house owns the flank cell, so no ground is pulled there.
    const auto flank = antwika::game::tileSpriteBounds(
        Cell{.x = 0, .y = 2}, scene_.camera);

    for (const auto &blit : renderer.blits)
    {
        EXPECT_FALSE(
            blit.source == groundTile()
            && blit.destination == flank);
    }
}

// A sprite overhangs its diamond, so paint order is screen depth.
// A building goes down with the diagonal its block starts on.
// That is after the ground behind it and before the ground in front.
// Which is what tucks its skirt under the cells south and east of it.
TEST_F(GridSceneTest, Draw_PaintsABuildingBetweenItsNeighbouringDiagonals)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 3, .height = 3});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 1, .y = 1},
            .kind = antwika::game::BuildingKind::House});

    scene.draw(renderer, kCanvas, scene_, atlases);

    // Eight ground cells round the house, and the house itself.
    ASSERT_EQ(renderer.blits.size(), 9U);

    const auto house =
        antwika::game::buildingTile(antwika::game::BuildingKind::House);

    // Five cells lie on the diagonals up to the house's own.
    // Three lie beyond it, and they are painted after it.
    EXPECT_EQ(renderer.blits[5].source, house);

    for (std::size_t blit = 0; blit < renderer.blits.size(); ++blit)
    {
        if (blit != 5)
        {
            EXPECT_EQ(renderer.blits[blit].source, groundTile())
                << "blit " << blit;
        }
    }
}
