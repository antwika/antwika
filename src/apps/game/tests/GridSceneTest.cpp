#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/SceneSnapshot.hpp"

using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::Direction;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::SceneSnapshot;
using antwika::game::WalkerView;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 640, .height = 480};

    // Records the calls, so the picture can be inspected as a whole.
    // Otherwise it could only be asserted call by call.
    class RecordingRenderer : public NiceMock<MockRenderer>
    {
    public:
        RecordingRenderer()
        {
            ON_CALL(*this, drawLine(_, _, _))
                .WillByDefault(
                    [this](Point from, Point to, Color color)
                    {
                        lines.push_back(Line{from, to, color});
                    });
            ON_CALL(*this, drawRect(_, _))
                .WillByDefault(
                    [this](Rect rect, Color color)
                    { rects.push_back(Fill{rect, color}); });
        }

        struct Line
        {
            Point from;
            Point to;
            Color color;
        };

        struct Fill
        {
            Rect rect;
            Color color;
        };

        [[nodiscard]] std::size_t linesOf(Color color) const
        {
            std::size_t count = 0;
            for (const auto &line : lines)
            {
                if (line.color == color)
                {
                    ++count;
                }
            }
            return count;
        }

        std::vector<Line> lines;
        std::vector<Fill> rects;
    };

    [[nodiscard]] SceneSnapshot snapshot(
        Camera camera,
        GridExtent extent,
        std::vector<Cell> paths = {},
        std::vector<WalkerView> walkers = {})
    {
        return SceneSnapshot{
            .camera = camera,
            .extent = extent,
            .paths = std::move(paths),
            .walkers = std::move(walkers)};
    }
} // namespace

TEST(GridSceneTest, Draw_ClearsThenLaysTheGroundBeforeAnythingElse)
{
    MockRenderer renderer;
    const GridScene scene;

    ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{.origin = {.x = 0, .y = 0}, .size = kCanvas}, _));
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());

    scene.draw(
        renderer,
        kCanvas,
        snapshot(Camera(Point{.x = 300, .y = 40}, 2),
                 GridExtent{.width = 2, .height = 2}));
}

TEST(GridSceneTest, Draw_DrawsTwoLatticeEdgesPerVisibleCell)
{
    RecordingRenderer renderer;
    const GridScene scene;
    constexpr GridExtent extent{.width = 3, .height = 3};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(Camera(Point{.x = 300, .y = 40}, 2), extent));

    // Two per cell, and no more.
    // The other two edges of each diamond belong to its neighbours.
    EXPECT_EQ(renderer.lines.size(), 2U * 3U * 3U);
}

TEST(GridSceneTest, Draw_FillsAPathCellWithLinesAcrossItsDiamond)
{
    RecordingRenderer renderer;
    const GridScene scene;
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell path{.x = 1, .y = 1};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(camera, GridExtent{.width = 3, .height = 3}, {path}));

    // One line per row of the diamond, top corner to bottom corner.
    const auto expected =
        2U * static_cast<std::size_t>(camera.halfHeight()) + 1U;
    EXPECT_EQ(
        renderer.linesOf(
            Color{.red = 176, .green = 150, .blue = 96}),
        expected);
}

TEST(GridSceneTest, Draw_CentresAPathFillOnItsCell)
{
    RecordingRenderer renderer;
    const GridScene scene;
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell path{.x = 1, .y = 1};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(camera, GridExtent{.width = 3, .height = 3}, {path}));

    const auto centre = cellCentre(path, camera);
    constexpr Color kPath{.red = 176, .green = 150, .blue = 96};

    // The widest of the tile's own spans runs through its middle.
    std::optional<RecordingRenderer::Line> widest;
    for (const auto &line : renderer.lines)
    {
        if (line.color != kPath)
        {
            continue;
        }

        if (!widest || line.to.x - line.from.x
                           > widest->to.x - widest->from.x)
        {
            widest = line;
        }
    }

    ASSERT_TRUE(widest.has_value());
    EXPECT_EQ(widest->from.y, centre.y);
    EXPECT_EQ(widest->to.y, centre.y);
}

TEST(GridSceneTest, Draw_MakesTheDiamondSymmetricAboutItsCentreRow)
{
    RecordingRenderer renderer;
    const GridScene scene;
    const Camera camera(Point{.x = 300, .y = 40}, 2);

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            camera,
            GridExtent{.width = 1, .height = 1},
            {Cell{.x = 0, .y = 0}}));

    const auto centre = cellCentre(Cell{.x = 0, .y = 0}, camera);
    constexpr Color kPath{.red = 176, .green = 150, .blue = 96};

    std::size_t spans = 0;
    for (const auto &line : renderer.lines)
    {
        if (line.color != kPath)
        {
            continue;
        }

        ++spans;

        // Every span is centred on the cell's column.
        EXPECT_EQ(line.from.x - centre.x, centre.x - line.to.x)
            << "row " << line.from.y;
    }

    EXPECT_GT(spans, 0U);
}

TEST(GridSceneTest, Draw_DrawsAWalkerSmallerThanItsTile)
{
    RecordingRenderer renderer;
    const GridScene scene;
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            camera,
            GridExtent{.width = 3, .height = 3},
            {where},
            {WalkerView{.at = where, .facing = Direction::East}}));

    const auto walkerRows =
        2U * static_cast<std::size_t>(camera.halfHeight() / 2) + 1U;
    const auto pathRows =
        2U * static_cast<std::size_t>(camera.halfHeight()) + 1U;

    EXPECT_LT(walkerRows, pathRows);
    EXPECT_EQ(
        renderer.linesOf(Color{.red = 232, .green = 200, .blue = 96}),
        walkerRows);
}

TEST(GridSceneTest, Draw_ColoursAWalkerByWhichWayItFaces)
{
    const Camera camera(Point{.x = 300, .y = 40}, 2);
    constexpr Cell where{.x = 1, .y = 1};

    std::vector<Color> seen;
    for (const auto facing : {
             Direction::North,
             Direction::East,
             Direction::South,
             Direction::West,
         })
    {
        RecordingRenderer renderer;
        const GridScene scene;

        scene.draw(
            renderer,
            kCanvas,
            snapshot(
                camera,
                GridExtent{.width = 3, .height = 3},
                {},
                {WalkerView{.at = where, .facing = facing}}));

        // Walkers are drawn last, so the final line is one of theirs.
        ASSERT_FALSE(renderer.lines.empty());
        seen.push_back(renderer.lines.back().color);
    }

    // Four facings, four distinct colours.
    for (std::size_t i = 0; i < seen.size(); ++i)
    {
        for (std::size_t j = i + 1; j < seen.size(); ++j)
        {
            EXPECT_NE(seen[i], seen[j]) << i << " vs " << j;
        }
    }
}

// The culling claim, asserted rather than assumed.
TEST(GridSceneTest, Draw_SkipsAPathCellEntirelyOffTheCanvas)
{
    RecordingRenderer renderer;
    const GridScene scene;

    // Panned far away, so nothing reaches the canvas at all.
    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = -100000, .y = -100000}, 2),
            GridExtent{.width = 4, .height = 4},
            {Cell{.x = 1, .y = 1}},
            {WalkerView{.at = Cell{.x = 2, .y = 2}}}));

    EXPECT_TRUE(renderer.lines.empty());
}

TEST(GridSceneTest, Draw_StillLaysTheGroundWhenEverythingIsCulled)
{
    RecordingRenderer renderer;
    const GridScene scene;

    scene.draw(
        renderer,
        kCanvas,
        snapshot(
            Camera(Point{.x = -100000, .y = -100000}, 2),
            GridExtent{.width = 4, .height = 4}));

    EXPECT_EQ(renderer.rects.size(), 1U);
}

// A canvas smaller than a tile is the underflow trap blog/012 found.
TEST(GridSceneTest, Draw_SurvivesACanvasSmallerThanOneTile)
{
    RecordingRenderer renderer;
    const GridScene scene;

    scene.draw(
        renderer,
        Size{.width = 1, .height = 1},
        snapshot(
            Camera(Point{}, 4),
            GridExtent{.width = 2, .height = 2},
            {Cell{.x = 0, .y = 0}}));

    // Nothing ran off into a four-billion-pixel line.
    for (const auto &line : renderer.lines)
    {
        EXPECT_LT(std::abs(line.to.x - line.from.x), 1000);
        EXPECT_LT(std::abs(line.to.y - line.from.y), 1000);
    }
}

TEST(GridSceneTest, Draw_HandlesAnExtentWithNoCells)
{
    RecordingRenderer renderer;
    const GridScene scene;

    scene.draw(
        renderer, kCanvas, snapshot(Camera(), GridExtent{}));

    EXPECT_TRUE(renderer.lines.empty());
    EXPECT_EQ(renderer.rects.size(), 1U);
}

TEST(GridSceneTest, Draw_PresentsNothingItself)
{
    MockRenderer renderer;
    const GridScene scene;

    // Presenting belongs to whatever owns the frame, not to the scene.
    EXPECT_CALL(renderer, present()).Times(0);
    EXPECT_CALL(renderer, clear(_)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());

    scene.draw(
        renderer, kCanvas, snapshot(Camera(), GridExtent{.width = 1,
                                                         .height = 1}));
}

// Culling has to reject a cell past every edge, not only the near ones.
TEST(GridSceneTest, Draw_SkipsCellsPastEachEdgeOfTheCanvas)
{
    for (const auto pan : {
             Point{.x = -100000, .y = 0},
             Point{.x = 0, .y = -100000},
             Point{.x = 100000, .y = 0},
             Point{.x = 0, .y = 100000},
         })
    {
        RecordingRenderer renderer;
        const GridScene scene;

        scene.draw(
            renderer,
            kCanvas,
            snapshot(
                Camera(pan, 2),
                GridExtent{.width = 2, .height = 2},
                {Cell{.x = 0, .y = 0}},
                {WalkerView{.at = Cell{.x = 1, .y = 1}}}));

        EXPECT_TRUE(renderer.lines.empty())
            << "pan " << pan.x << "," << pan.y;
    }
}
