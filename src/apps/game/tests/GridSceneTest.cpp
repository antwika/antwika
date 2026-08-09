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

#include "Translators.hpp"
#include "AtlasSpecsFixture.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/OverlayLabel.hpp"
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/ResourceColour.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::game::testing::kTestSpecs;
using antwika::game::tests::kTranslator;

using antwika::animation::Progress;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::Direction;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::groundTile;
using antwika::game::kTicksPerStep;
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

    enum class Call : std::uint8_t
    {
        Blit,
        Rect,
        Text,
    };

    class RecordingRenderer final : public NiceMock<MockRenderer>
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

        struct Filled final
        {
            Rect rect;
            Color color;

            [[nodiscard]] bool operator==(const Filled &other) const
                = default;
        };

        struct Written final
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

        struct Blit final
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
            .ruins = {},
            .plan = {},
            .ghost = {},
            .hover = {},
            .overlay = {}};
    }
}

class GridSceneTest : public ::testing::Test
{
protected:
    RecordingRenderer renderer;

    NiceMock<MockTexture> atlas;
    NiceMock<MockTexture> atlas2x2;
    NiceMock<MockTexture> atlas3x3;
    NiceMock<MockTexture> walkerAtlas;
    antwika::game::AtlasTextures atlases{
        .oneByOne = atlas,
        .twoByTwo = atlas2x2,
        .threeByThree = atlas3x3,
        .walker = walkerAtlas,
        .specs = kTestSpecs};

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

    EXPECT_EQ(renderer.blitsOf(groundTile(kTestSpecs)), 3U * 3U);
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

    ASSERT_EQ(renderer.blits.size(), 6U);

    EXPECT_EQ(
        renderer.blits[4].source,
        roadTile(kTestSpecs, 0));
    EXPECT_EQ(
        renderer.blits[4].destination,
        antwika::game::tileSpriteBounds(kTestSpecs, where, camera));
    EXPECT_EQ(
        renderer.blits[5].source, walkerTile(kTestSpecs, Direction::East));
    EXPECT_EQ(
        renderer.blits[5].destination,
        antwika::game::tileSpriteBounds(kTestSpecs, where, camera));
}

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
        EXPECT_EQ(blit.tint, kUntinted);
    }
}

TEST_F(GridSceneTest, Draw_TakesTheGroundFromTheSheetOfItsOwnSize)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 2, .height = 2},
            {Cell{.x = 0, .y = 0}}),
        atlases);

    ASSERT_FALSE(renderer.blits.empty());

    for (const auto &blit : renderer.blits)
    {
        EXPECT_EQ(blit.texture, &atlas);
    }
}

TEST_F(GridSceneTest, Draw_MarchesAWalkerOnTheWalkersOwnSheet)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{},
            {},
            {WalkerSprite{.at = Cell{.x = 0, .y = 0}}}),
        atlases);

    ASSERT_EQ(renderer.blits.size(), 1U);
    EXPECT_EQ(renderer.blits.front().texture, &walkerAtlas);
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

    EXPECT_EQ(renderer.blitsOf(roadTile(kTestSpecs, 0)), 1U);
}

TEST_F(GridSceneTest, Draw_ChoosesARoadTileFromTheNeighboursItHas)
{
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
            roadTile(
                kTestSpecs,
                linkBit(Direction::East) | linkBit(Direction::West))),
        1U);

    EXPECT_EQ(renderer.blitsOf(roadTile(
        kTestSpecs,
        linkBit(Direction::East))), 1U);
    EXPECT_EQ(renderer.blitsOf(roadTile(
        kTestSpecs,
        linkBit(Direction::West))), 1U);
}

TEST_F(GridSceneTest, Draw_ChoosesTheJunctionTileWhereFourRoadsMeet)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{.width = 4, .height = 4},
            {Cell{.x = 0, .y = 1},
             Cell{.x = 1, .y = 0},
             Cell{.x = 1, .y = 1},
             Cell{.x = 1, .y = 2},
             Cell{.x = 2, .y = 1}}),
        atlases);

    EXPECT_EQ(
        renderer.blitsOf(roadTile(kTestSpecs, antwika::game::kLinkMask)), 1U);
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
        EXPECT_EQ(each.blits.front().source, walkerTile(kTestSpecs, facing));
    }
}

TEST_F(GridSceneTest, Draw_CyclesAWalkersLegsAsItCrossesACell)
{
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = 300, .y = 40}, 2),
            GridExtent{},
            {},
            {WalkerSprite{
                .at = Cell{.x = 1, .y = 1},
                .facing = Direction::East,
                .from = Cell{.x = 0, .y = 1},
                .ticksIntoStep = 3 * kTicksPerStep / 4}}),
        atlases,
        Progress(1, 2));

    ASSERT_EQ(renderer.blits.size(), 1U);
    EXPECT_EQ(
        renderer.blits.front().source, walkerTile(
            kTestSpecs,
            Direction::East, 3));
}

TEST_F(GridSceneTest, Draw_HoldsAPausedWalkersLegsStill)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{},
        {},
        {WalkerSprite{
            .at = Cell{.x = 1, .y = 1},
            .facing = Direction::East,
            .from = Cell{.x = 0, .y = 1},
            .ticksIntoStep = kTicksPerStep / 2}});
    scene_.paused = true;

    scene.draw(renderer, kCanvas, scene_, atlases, Progress(1, 2));

    ASSERT_EQ(renderer.blits.size(), 1U);
    EXPECT_EQ(
        renderer.blits.front().source, walkerTile(
            kTestSpecs,
            Direction::East, 2));
}

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
    EXPECT_EQ(renderer.blits[0].source, groundTile(kTestSpecs));
    EXPECT_EQ(renderer.blits[1].source, roadTile(kTestSpecs, 0));
    EXPECT_EQ(
        renderer.blits[2].source, walkerTile(kTestSpecs, Direction::North));
}

TEST_F(GridSceneTest, Draw_SkipsEverythingEntirelyOffTheCanvas)
{
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

        EXPECT_TRUE(each.blits.empty()) << pan.x << ' ' << pan.y;
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

    for (const auto &blit : renderer.blits)
    {
        EXPECT_LT(blit.destination.size.width, 1000U);
        EXPECT_LT(blit.destination.size.height, 1000U);
    }
}

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
        EXPECT_EQ(each.blits.front().source, groundTile(kTestSpecs));
        EXPECT_EQ(
            each.blits.front().destination,
            antwika::game::tileSpriteBounds(
                kTestSpecs,
                Cell{.x = 0, .y = 0}, camera));
    }
}

TEST_F(GridSceneTest, Draw_PresentsNothingItself)
{
    MockRenderer strict;

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

    EXPECT_EQ(start, antwika::game::tileSpriteBounds(kTestSpecs, from, camera));
    EXPECT_NE(middle, start);
    EXPECT_NE(middle, antwika::game::tileSpriteBounds(kTestSpecs, to, camera));
}

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

    EXPECT_NE(atTick, antwika::game::tileSpriteBounds(
        kTestSpecs,
        from, camera));
    EXPECT_NE(atTick, antwika::game::tileSpriteBounds(kTestSpecs, to, camera));
}

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

    EXPECT_EQ(renderer.blits, atTick);
}

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

TEST_F(GridSceneTest, Draw_PaintsNoOverlayForTheCityItself)
{
    const auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2});

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_EQ(renderer.blitsOf(groundTile(kTestSpecs)), 4U);
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

    EXPECT_EQ(scrimmed, 4U);
    EXPECT_EQ(tinted, 1U);
}

TEST_F(GridSceneTest, Draw_WritesEveryOverlayValueOnItsOwnTile)
{
    const Camera camera(Point{.x = 300, .y = 40}, 3);

    auto scene_ = snapshot(camera, GridExtent{.width = 2, .height = 2});
    scene_.view = antwika::game::MapView::Desirability;
    scene_.overlay = {
        {Cell{.x = 0, .y = 1}, 42}, {Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    ASSERT_EQ(renderer.texts.size(), 2U);

    for (const auto &written : renderer.texts)
    {
        const auto cell = written.text == "42" ? Cell{.x = 0, .y = 1}
                                               : Cell{.x = 1, .y = 1};

        const auto label = antwika::game::overlayLabelFor(
            written.text, cell, camera);

        ASSERT_TRUE(label.has_value());
        EXPECT_EQ(written.origin, label->origin);
        EXPECT_EQ(written.scale, label->scale);
        EXPECT_EQ(written.color, antwika::game::kOverlayInk);
    }
}

TEST_F(GridSceneTest, Draw_WritesNoValueOnATileTheOverlayPassedOver)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2});
    scene_.view = antwika::game::MapView::Water;
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 7}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    ASSERT_EQ(renderer.texts.size(), 1U);
    EXPECT_EQ(renderer.texts[0].text, "7");
}

TEST_F(GridSceneTest, Draw_WritesNoValuesWithNoOverlayOpen)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2});
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_TRUE(renderer.texts.empty());
}

TEST_F(GridSceneTest, Draw_WritesNoValueForATileOffTheCanvas)
{
    auto scene_ = snapshot(
        Camera(Point{.x = -4000, .y = -4000}, 3),
        GridExtent{.width = 2, .height = 2});
    scene_.view = antwika::game::MapView::Desirability;
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_TRUE(renderer.texts.empty());
}

TEST_F(GridSceneTest, Draw_WritesNoValueTooBigForItsTile)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 1),
        GridExtent{.width = 2, .height = 2});
    scene_.view = antwika::game::MapView::Desirability;
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_TRUE(renderer.texts.empty());
}

TEST_F(GridSceneTest, Draw_WritesTheOverlayValuesOverTheWalkers)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 2, .height = 2},
        {},
        {WalkerSprite{.at = Cell{.x = 1, .y = 1}}});
    scene_.view = antwika::game::MapView::Water;
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    ASSERT_EQ(renderer.texts.size(), 1U);
    EXPECT_EQ(renderer.order.back(), Call::Text);
}

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

    std::size_t scrims = 0;
    std::size_t lastScrim = 0;
    std::size_t walker = 0;

    for (std::size_t index = 0; index < renderer.blits.size(); ++index)
    {
        if (renderer.blits[index].tint == antwika::game::kOverlayScrim)
        {
            ++scrims;
            lastScrim = index;
        }

        if (renderer.blits[index].source
            == walkerTile(kTestSpecs, Direction::East))
        {
            walker = index;
        }
    }

    ASSERT_GT(scrims, 0U);
    ASSERT_GT(walker, 0U);
    EXPECT_GT(walker, lastScrim);
}

TEST_F(GridSceneTest, Draw_PaintsNoOverlayCellOffTheCanvas)
{
    auto scene_ = snapshot(
        Camera(Point{.x = -100000, .y = -100000}, 3),
        GridExtent{.width = 2, .height = 2});
    scene_.view = antwika::game::MapView::Food;
    scene_.overlay = {{Cell{.x = 1, .y = 1}, 100}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_TRUE(renderer.blits.empty());
}

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

    ASSERT_EQ(panel.lines.size(), 11 + antwika::game::kResourceCount);
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

    ASSERT_FALSE(renderer.rects.empty());
    EXPECT_EQ(renderer.rects.back().rect, panel.box);
    EXPECT_EQ(renderer.rects.back().color, antwika::game::kReadoutBackdrop);
    EXPECT_EQ(renderer.order.back(), Call::Text);
}

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

TEST_F(GridSceneTest, Draw_PreviewsAPlannedBlockAsTheHousesItWouldLay)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 4, .height = 4});
    scene_.ghost.tool = antwika::game::BuildTool::House;
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

    const std::vector<Rect> wanted(
        3U,
        antwika::game::buildingTile(kTestSpecs, 
            antwika::game::BuildingKind::House));

    EXPECT_EQ(previewed, wanted);
}

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

    const std::vector<Rect> wanted{
        roadTile(kTestSpecs, antwika::game::linkBit(Direction::East)),
        roadTile(kTestSpecs, 
            antwika::game::linkBit(Direction::East)
            | antwika::game::linkBit(Direction::West)),
        roadTile(kTestSpecs, antwika::game::linkBit(Direction::West))};

    EXPECT_EQ(previewed, wanted);
}

TEST_F(GridSceneTest, Draw_JoinsAPlannedRunOntoTheRoadsAlreadyThere)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 3),
        GridExtent{.width = 4, .height = 4},
        {Cell{.x = 1, .y = 1}});
    scene_.plan = antwika::game::RoadPlan{
        .cells = {Cell{.x = 2, .y = 1}}, .valid = true};

    scene.draw(renderer, kCanvas, scene_, atlases);

    std::size_t previewed = 0;

    for (const auto &blit : renderer.blits)
    {
        if (blit.tint.alpha == 110 && blit.tint.green == 255)
        {
            ++previewed;
            EXPECT_EQ(
                blit.source,
                roadTile(kTestSpecs, antwika::game::linkBit(Direction::West)));
        }
    }

    EXPECT_EQ(previewed, 1U);
}

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
            antwika::game::buildingTile(kTestSpecs, expected.kind));
        EXPECT_EQ(
            each.blits.front().destination,
            antwika::game::buildingSpriteBounds(kTestSpecs, 
                Cell{.x = 0, .y = 0}, expected.kind, camera));
    }
}

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

    ASSERT_EQ(renderer.blits.size(), 1U);
    EXPECT_EQ(
        renderer.blits.front().source,
        antwika::game::buildingTile(
            kTestSpecs,
            antwika::game::BuildingKind::Farm));
}

TEST_F(GridSceneTest, Draw_PaintsAWideBlocksFlankBehindItsArt)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 5, .height = 5});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 1, .y = 0},
            .kind = antwika::game::BuildingKind::Storage});

    scene_.paths.push_back(Cell{.x = 0, .y = 2});

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto storage = antwika::game::buildingTile(kTestSpecs, 
        antwika::game::BuildingKind::Storage);

    std::size_t storageAt = renderer.blits.size();
    std::size_t roadAt = renderer.blits.size();

    for (std::size_t blit = 0; blit < renderer.blits.size(); ++blit)
    {
        if (renderer.blits[blit].source == storage)
        {
            storageAt = blit;
        }

        if (renderer.blits[blit].source == roadTile(kTestSpecs, 0))
        {
            roadAt = blit;
        }
    }

    ASSERT_LT(storageAt, renderer.blits.size());
    ASSERT_LT(roadAt, renderer.blits.size());
    EXPECT_LT(roadAt, storageAt);

    EXPECT_EQ(renderer.blitsOf(roadTile(kTestSpecs, 0)), 1U);
}

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

    const auto shared = antwika::game::tileSpriteBounds(kTestSpecs, 
        Cell{.x = 2, .y = 2}, scene_.camera);

    std::size_t drawn = 0;
    std::size_t sharedAt = 0;
    std::size_t firstBlock = renderer.blits.size();

    for (std::size_t blit = 0; blit < renderer.blits.size(); ++blit)
    {
        if (renderer.blits[blit].source == groundTile(kTestSpecs)
            && renderer.blits[blit].destination == shared)
        {
            ++drawn;
            sharedAt = blit;
        }

        if (renderer.blits[blit].source
                == antwika::game::buildingTile(kTestSpecs, 
                    antwika::game::BuildingKind::Storage)
            && blit < firstBlock)
        {
            firstBlock = blit;
        }
    }

    EXPECT_EQ(drawn, 1U);
    ASSERT_LT(firstBlock, renderer.blits.size());
    EXPECT_LT(sharedAt, firstBlock);
}

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

    const auto flank = antwika::game::tileSpriteBounds(kTestSpecs, 
        Cell{.x = 0, .y = 2}, scene_.camera);

    ASSERT_EQ(
        renderer.blitsOf(
            antwika::game::buildingTile(kTestSpecs, 
                antwika::game::BuildingKind::House)),
        1U);

    for (const auto &blit : renderer.blits)
    {
        EXPECT_FALSE(
            blit.source == groundTile(kTestSpecs)
            && blit.destination == flank);
    }
}

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

    ASSERT_EQ(renderer.blits.size(), 9U);

    const auto house =
        antwika::game::buildingTile(
            kTestSpecs,
            antwika::game::BuildingKind::House);

    EXPECT_EQ(renderer.blits[5].source, house);

    for (std::size_t blit = 0; blit < renderer.blits.size(); ++blit)
    {
        if (blit != 5)
        {
            EXPECT_EQ(
                renderer.blits[blit].source, groundTile(kTestSpecs))
                << blit;
        }
    }
}

TEST_F(GridSceneTest, Draw_BlitsARuinFromItsKindsOwnSheet)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 3, .height = 3});
    scene_.ruins = {
        antwika::game::RuinView{
            .at = Cell{.x = 0, .y = 0},
            .kind = antwika::game::BuildingKind::Farm,
            .state = antwika::game::RuinState::Burning}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_EQ(renderer.blitsOf(groundTile(kTestSpecs)), 5U);

    const auto fireTile = antwika::game::ruinTile(kTestSpecs, 
        antwika::game::RuinState::Burning,
        antwika::game::BuildingKind::Farm);

    ASSERT_EQ(renderer.blitsOf(fireTile), 1U);

    for (const auto &blit : renderer.blits)
    {
        if (blit.source == fireTile)
        {
            EXPECT_EQ(blit.texture, &atlas2x2);
        }
    }
}

TEST_F(GridSceneTest, Draw_BlitsDebrisWhereTheFireHasGoneOut)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 2, .height = 2});
    scene_.ruins = {
        antwika::game::RuinView{
            .at = Cell{.x = 1, .y = 1},
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto debrisTile = antwika::game::ruinTile(kTestSpecs, 
        antwika::game::RuinState::Debris,
        antwika::game::BuildingKind::House);

    ASSERT_EQ(renderer.blitsOf(debrisTile), 1U);

    for (const auto &blit : renderer.blits)
    {
        if (blit.source == debrisTile)
        {
            EXPECT_EQ(blit.texture, &atlas);
        }
    }
}

TEST_F(GridSceneTest, Draw_PaintsRuinsAndBuildingsBackToFront)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 4, .height = 4});
    scene_.buildings = {
        antwika::game::BuildingSprite{
            .at = Cell{.x = 2, .y = 2},
            .kind = antwika::game::BuildingKind::House}};
    scene_.ruins = {
        antwika::game::RuinView{
            .at = Cell{.x = 1, .y = 1},
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto debrisTile = antwika::game::ruinTile(kTestSpecs, 
        antwika::game::RuinState::Debris,
        antwika::game::BuildingKind::House);
    const auto houseTile = antwika::game::buildingTile(kTestSpecs, 
        antwika::game::BuildingKind::House);

    std::size_t debrisAt = 0;
    std::size_t houseAt = 0;

    for (std::size_t index = 0; index < renderer.blits.size(); ++index)
    {
        if (renderer.blits[index].source == debrisTile)
        {
            debrisAt = index;
        }

        if (renderer.blits[index].source == houseTile)
        {
            houseAt = index;
        }
    }

    EXPECT_LT(debrisAt, houseAt);
}

TEST_F(GridSceneTest, Draw_MergesATiedDiagonalByAscendingX)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 4, .height = 4});
    scene_.buildings = {
        antwika::game::BuildingSprite{
            .at = Cell{.x = 0, .y = 2},
            .kind = antwika::game::BuildingKind::House}};
    scene_.ruins = {
        antwika::game::RuinView{
            .at = Cell{.x = 2, .y = 0},
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris},
        antwika::game::RuinView{
            .at = Cell{.x = 1, .y = 3},
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto houseTile = antwika::game::buildingTile(kTestSpecs, 
        antwika::game::BuildingKind::House);
    const auto debrisTile = antwika::game::ruinTile(kTestSpecs, 
        antwika::game::RuinState::Debris,
        antwika::game::BuildingKind::House);

    std::size_t houseAt = 0;
    std::size_t firstDebris = renderer.blits.size();

    for (std::size_t index = 0; index < renderer.blits.size(); ++index)
    {
        if (renderer.blits[index].source == houseTile)
        {
            houseAt = index;
        }

        if (renderer.blits[index].source == debrisTile
            && index < firstDebris)
        {
            firstDebris = index;
        }
    }

    ASSERT_EQ(renderer.blitsOf(houseTile), 1U);
    ASSERT_LT(firstDebris, renderer.blits.size());
    EXPECT_LT(houseAt, firstDebris);
    EXPECT_EQ(renderer.blitsOf(debrisTile), 2U);
}

TEST_F(GridSceneTest, Draw_MergesATiedDiagonalRuinFirst)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 4, .height = 4});
    scene_.buildings = {
        antwika::game::BuildingSprite{
            .at = Cell{.x = 2, .y = 0},
            .kind = antwika::game::BuildingKind::House}};
    scene_.ruins = {
        antwika::game::RuinView{
            .at = Cell{.x = 0, .y = 2},
            .kind = antwika::game::BuildingKind::House,
            .state = antwika::game::RuinState::Debris}};

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto houseTile = antwika::game::buildingTile(kTestSpecs, 
        antwika::game::BuildingKind::House);
    const auto debrisTile = antwika::game::ruinTile(kTestSpecs, 
        antwika::game::RuinState::Debris,
        antwika::game::BuildingKind::House);

    std::size_t houseAt = 0;
    std::size_t debrisAt = 0;

    for (std::size_t index = 0; index < renderer.blits.size(); ++index)
    {
        if (renderer.blits[index].source == houseTile)
        {
            houseAt = index;
        }

        if (renderer.blits[index].source == debrisTile)
        {
            debrisAt = index;
        }
    }

    EXPECT_LT(debrisAt, houseAt);
}

namespace
{
    [[nodiscard]] std::size_t blitIndexOf(
        const RecordingRenderer &recorded, Rect source)
    {
        for (std::size_t index = 0; index < recorded.blits.size();
             ++index)
        {
            if (recorded.blits[index].source == source)
            {
                return index;
            }
        }

        return recorded.blits.size();
    }
}

TEST_F(GridSceneTest, Draw_HidesAWalkerBehindTheBlockInFrontOfIt)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 6, .height = 6},
        {},
        {WalkerSprite{
            .at = Cell{.x = 1, .y = 1}, .facing = Direction::North}});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 2, .y = 2},
            .kind = antwika::game::BuildingKind::Farm});

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto walker = blitIndexOf(
        renderer, walkerTile(kTestSpecs, Direction::North));
    const auto farm = blitIndexOf(
        renderer,
        antwika::game::buildingTile(
            kTestSpecs,
            antwika::game::BuildingKind::Farm));

    ASSERT_LT(walker, renderer.blits.size());
    ASSERT_LT(farm, renderer.blits.size());
    EXPECT_LT(walker, farm);
}

TEST_F(GridSceneTest, Draw_KeepsAWalkerInFrontOfTheBlockBehindIt)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 6, .height = 6},
        {},
        {WalkerSprite{
            .at = Cell{.x = 4, .y = 4}, .facing = Direction::North}});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 2, .y = 2},
            .kind = antwika::game::BuildingKind::Farm});

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto walker = blitIndexOf(
        renderer, walkerTile(kTestSpecs, Direction::North));
    const auto farm = blitIndexOf(
        renderer,
        antwika::game::buildingTile(
            kTestSpecs,
            antwika::game::BuildingKind::Farm));

    ASSERT_LT(walker, renderer.blits.size());
    ASSERT_LT(farm, renderer.blits.size());
    EXPECT_LT(farm, walker);
}

TEST_F(GridSceneTest, Draw_PaintsAMidStepWalkerAtItsDeeperCell)
{
    auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 6, .height = 6},
        {},
        {WalkerSprite{
            .at = Cell{.x = 2, .y = 1},
            .facing = Direction::North,
            .from = Cell{.x = 3, .y = 1},
            .ticksIntoStep = 0}});
    scene_.buildings.push_back(
        antwika::game::BuildingSprite{
            .at = Cell{.x = 2, .y = 2},
            .kind = antwika::game::BuildingKind::Farm});

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto walker = blitIndexOf(
        renderer, walkerTile(kTestSpecs, Direction::North));
    const auto farm = blitIndexOf(
        renderer,
        antwika::game::buildingTile(
            kTestSpecs,
            antwika::game::BuildingKind::Farm));

    ASSERT_LT(walker, renderer.blits.size());
    ASSERT_LT(farm, renderer.blits.size());
    EXPECT_LT(farm, walker);
}

TEST_F(GridSceneTest, Draw_PaintsWalkersBackToFront)
{
    const auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 6, .height = 6},
        {},
        {WalkerSprite{
             .at = Cell{.x = 3, .y = 3}, .facing = Direction::North},
         WalkerSprite{
             .at = Cell{.x = 0, .y = 0}, .facing = Direction::East}});

    scene.draw(renderer, kCanvas, scene_, atlases);

    const auto shallow = blitIndexOf(
        renderer, walkerTile(kTestSpecs, Direction::East));
    const auto deep = blitIndexOf(
        renderer, walkerTile(kTestSpecs, Direction::North));

    ASSERT_LT(shallow, renderer.blits.size());
    ASSERT_LT(deep, renderer.blits.size());
    EXPECT_LT(shallow, deep);
}

TEST_F(GridSceneTest, Draw_PaintsAWalkerHandedFromPastTheExtent)
{
    const auto scene_ = snapshot(
        Camera(Point{.x = 300, .y = 40}, 2),
        GridExtent{.width = 2, .height = 2},
        {},
        {WalkerSprite{
            .at = Cell{.x = 3, .y = 3}, .facing = Direction::North}});

    scene.draw(renderer, kCanvas, scene_, atlases);

    EXPECT_LT(
        blitIndexOf(renderer, walkerTile(kTestSpecs, Direction::North)),
        renderer.blits.size());
}
