#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/SizeF.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/mapcheck/Finding.hpp>
#include <antwika/mapcheck/MapReport.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/OverlayDraw.hpp"
#include "antwika/map_editor/PaletteMath.hpp"

using antwika::geometry::GridCell;
using antwika::gfx::Color;
using antwika::gfx::PointF;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::SizeF;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::map_editor::CellSpan;
using antwika::map_editor::chromeFor;
using antwika::map_editor::drawActiveLevelMarks;
using antwika::map_editor::drawFreeMark;
using antwika::map_editor::drawHover;
using antwika::map_editor::drawMapSelectionOverlay;
using antwika::map_editor::drawMarker;
using antwika::map_editor::drawSelection;
using antwika::map_editor::drawValidatorOverlay;
using antwika::map_editor::EditorState;
using antwika::map_editor::EditorStore;
using antwika::map_editor::MapCamera;
using antwika::map_editor::MarkerKind;
using antwika::mapcheck::CellReach;
using antwika::mapcheck::Finding;
using antwika::mapcheck::MapReport;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Overlay;
using antwika::tilemap::Rgb;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using ::testing::_;
using ::testing::Contains;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 320, .height = 270};

    constexpr Color kWhite{.red = 255, .green = 255, .blue = 255};

    constexpr Color kUnreachable{
        .red = 255, .green = 0, .blue = 0, .alpha = 96};

    constexpr Color kFinding{.red = 255, .green = 0, .blue = 0};

    constexpr Rgb kSlate{.red = 32, .green = 40, .blue = 48};

    struct DrawnRect final
    {
        RectF rect{};
        Color color{};

        [[nodiscard]] bool operator==(const DrawnRect &other) const
            = default;
    };

    struct DrawnLine final
    {
        PointF from{};
        PointF to{};
        Color color{};

        [[nodiscard]] bool operator==(const DrawnLine &other) const
            = default;
    };

    class Recorder final
    {
    public:
        Recorder()
        {
            ON_CALL(inner, drawRect(_, _))
                .WillByDefault(
                    [this](const RectF rect, const Color color)
                    { rects.push_back(DrawnRect{rect, color}); });
            ON_CALL(inner, drawLine(_, _, _))
                .WillByDefault(
                    [this](
                        const PointF from,
                        const PointF to,
                        const Color color) {
                        lines.push_back(
                            DrawnLine{from, to, color});
                    });
        }

        [[nodiscard]] ViewportRenderer &view() noexcept
        {
            return renderer;
        }

        std::vector<DrawnRect> rects{};
        std::vector<DrawnLine> lines{};

    private:
        NiceMock<MockRenderer> inner{};
        ViewportRenderer renderer{inner, kCanvas, kCanvas};
    };

    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] RectF rectAt(
        const float x,
        const float y,
        const float width,
        const float height)
    {
        return RectF{PointF{x, y}, SizeF{width, height}};
    }

    [[nodiscard]] std::vector<DrawnLine> outlineOf(
        const RectF rect, const Color color)
    {
        const auto left = rect.origin.x;
        const auto top = rect.origin.y;
        const auto right = left + rect.size.width;
        const auto bottom = top + rect.size.height;

        return {
            DrawnLine{{left, top}, {right, top}, color},
            DrawnLine{{right, top}, {right, bottom}, color},
            DrawnLine{{right, bottom}, {left, bottom}, color},
            DrawnLine{{left, bottom}, {left, top}, color}};
    }

    [[nodiscard]] EditorState stateOf(
        const std::uint32_t columns, const std::uint32_t rows)
    {
        EditorState state{.map = TileMap{MapHeader{}, columns, rows}};
        state.overlayOn = true;
        state.report = MapReport{};
        state.report->reachable.assign(
            static_cast<std::size_t>(columns)
                * static_cast<std::size_t>(rows),
            CellReach{.anyStandable = true, .anyReached = true});

        return state;
    }

    [[nodiscard]] EditorState strandedColumnOf(
        const std::vector<Slab> &slabs)
    {
        auto state = stateOf(1, 1);
        auto &column = state.map.at(cellAt(0, 0));

        column.clear();

        for (const auto &slab : slabs)
        {
            static_cast<void>(column.place(slab));
        }

        state.report->reachable[0] =
            CellReach{.anyStandable = true, .anyReached = false};

        return state;
    }

    [[nodiscard]] EditorStore storeOf()
    {
        return EditorStore{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
    }
}

TEST(OverlayDrawTest, DrawMarker_FillsASmallSquareInTheKindsInk)
{
    Recorder canvas;

    drawMarker(
        canvas.view(), cellAt(2, 1), 0, MarkerKind::Spawn,
        MapCamera{});

    EXPECT_EQ(
        canvas.rects,
        (std::vector<DrawnRect>{
            DrawnRect{
                rectAt(38.0F, 32.0F, 4.0F, 4.0F),
                Color{.red = 255, .green = 0, .blue = 255}}}));
    EXPECT_TRUE(canvas.lines.empty());
}

TEST(OverlayDrawTest, DrawMarker_OutlinesATriggerInsteadOfFillingIt)
{
    Recorder canvas;

    drawMarker(
        canvas.view(), cellAt(0, 0), 0, MarkerKind::Trigger,
        MapCamera{});

    EXPECT_EQ(
        canvas.lines,
        outlineOf(
            rectAt(6.0F, 16.0F, 4.0F, 4.0F),
            Color{.red = 0, .green = 255, .blue = 0}));
    EXPECT_TRUE(canvas.rects.empty());
}

TEST(OverlayDrawTest, DrawMarker_LiftsTheSquareByTheLevelRise)
{
    Recorder canvas;

    drawMarker(
        canvas.view(), cellAt(0, 0), 2, MarkerKind::Boat,
        MapCamera{});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(canvas.rects[0].rect, rectAt(6.0F, 0.0F, 4.0F, 4.0F));
}

TEST(OverlayDrawTest, DrawMarker_ScalesAndPansWithTheCamera)
{
    Recorder canvas;
    const MapCamera camera{.step = 3, .panX = 5.0F, .panY = 7.0F};

    drawMarker(
        canvas.view(), cellAt(1, 0), 0, MarkerKind::Npc, camera);

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(
        canvas.rects[0].rect, rectAt(71.0F, 35.0F, 12.0F, 12.0F));
}

TEST(OverlayDrawTest, DrawSelection_OutlinesTheCellInWhite)
{
    Recorder canvas;

    drawSelection(canvas.view(), cellAt(1, 2), 0, MapCamera{});

    EXPECT_EQ(
        canvas.lines,
        outlineOf(rectAt(16.0F, 42.0F, 16.0F, 16.0F), kWhite));
    EXPECT_TRUE(canvas.rects.empty());
}

TEST(OverlayDrawTest, DrawHover_RingsTheCellOnePixelThickAtOneZoom)
{
    Recorder canvas;
    const auto ring = antwika::ui::Theme{}.focusRing;

    drawHover(canvas.view(), cellAt(0, 0), 0, MapCamera{});

    EXPECT_EQ(
        canvas.rects,
        (std::vector<DrawnRect>{
            DrawnRect{rectAt(0.0F, 10.0F, 16.0F, 1.0F), ring},
            DrawnRect{rectAt(0.0F, 25.0F, 16.0F, 1.0F), ring},
            DrawnRect{rectAt(0.0F, 11.0F, 1.0F, 14.0F), ring},
            DrawnRect{rectAt(15.0F, 11.0F, 1.0F, 14.0F), ring}}));
}

TEST(OverlayDrawTest, DrawHover_ThickensTheRingWithTheZoom)
{
    Recorder canvas;
    const auto ring = antwika::ui::Theme{}.focusRing;

    drawHover(
        canvas.view(), cellAt(0, 0), 0, MapCamera{.step = 2});

    EXPECT_EQ(
        canvas.rects,
        (std::vector<DrawnRect>{
            DrawnRect{rectAt(0.0F, 10.0F, 32.0F, 2.0F), ring},
            DrawnRect{rectAt(0.0F, 40.0F, 32.0F, 2.0F), ring},
            DrawnRect{rectAt(0.0F, 12.0F, 2.0F, 28.0F), ring},
            DrawnRect{rectAt(30.0F, 12.0F, 2.0F, 28.0F), ring}}));
}

TEST(OverlayDrawTest, DrawHover_KeepsAOnePixelRingWhenZoomedOut)
{
    Recorder canvas;
    const auto ring = antwika::ui::Theme{}.focusRing;

    drawHover(
        canvas.view(), cellAt(0, 0), 0, MapCamera{.step = 0});

    EXPECT_EQ(
        canvas.rects,
        (std::vector<DrawnRect>{
            DrawnRect{rectAt(0.0F, 10.0F, 8.0F, 1.0F), ring},
            DrawnRect{rectAt(0.0F, 17.0F, 8.0F, 1.0F), ring},
            DrawnRect{rectAt(0.0F, 11.0F, 1.0F, 6.0F), ring},
            DrawnRect{rectAt(7.0F, 11.0F, 1.0F, 6.0F), ring}}));
}

TEST(OverlayDrawTest, DrawFreeMark_TicksTheCellCornerInThePaperChrome)
{
    Recorder canvas;
    const TileMap map{MapHeader{.paper = kSlate}, 4, 4};

    drawFreeMark(canvas.view(), map, cellAt(1, 1), 0, MapCamera{});

    EXPECT_EQ(
        canvas.rects,
        (std::vector<DrawnRect>{
            DrawnRect{
                rectAt(17.0F, 27.0F, 2.0F, 2.0F),
                chromeFor(kSlate).freeMark}}));
}

TEST(
    OverlayDrawTest,
    DrawFreeMark_ShrinksTheTickToOnePixelWhenZoomedOut)
{
    Recorder canvas;
    const TileMap map{MapHeader{}, 4, 4};

    drawFreeMark(
        canvas.view(), map, cellAt(0, 0), 0, MapCamera{.step = 0});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(canvas.rects[0].rect, rectAt(0.5F, 10.5F, 1.0F, 1.0F));
}

TEST(OverlayDrawTest, DrawValidatorOverlay_DrawsNothingWhileTheOverlayIsOff)
{
    Recorder canvas;
    auto state = stateOf(1, 1);

    state.overlayOn = false;
    state.report->reachable[0] =
        CellReach{.anyStandable = true, .anyReached = false};

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_TRUE(canvas.rects.empty());
    EXPECT_TRUE(canvas.lines.empty());
}

TEST(OverlayDrawTest, DrawValidatorOverlay_DrawsNothingWithoutAReport)
{
    Recorder canvas;
    auto state = stateOf(1, 1);

    state.report.reset();

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_TRUE(canvas.rects.empty());
    EXPECT_TRUE(canvas.lines.empty());
}

TEST(OverlayDrawTest, DrawValidatorOverlay_ShadesAStandableCellNobodyReached)
{
    Recorder canvas;
    auto state = stateOf(2, 1);

    state.report->reachable[1] =
        CellReach{.anyStandable = true, .anyReached = false};

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_EQ(
        canvas.rects,
        (std::vector<DrawnRect>{
            DrawnRect{
                rectAt(16.0F, 10.0F, 16.0F, 16.0F), kUnreachable}}));
}

TEST(
    OverlayDrawTest,
    DrawValidatorOverlay_LeavesACellWithNoStandableSurfaceAlone)
{
    Recorder canvas;
    auto state = stateOf(1, 1);

    state.report->reachable[0] =
        CellReach{.anyStandable = false, .anyReached = false};

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_TRUE(canvas.rects.empty());
}

TEST(OverlayDrawTest, DrawValidatorOverlay_LeavesACellPastTheReachListAlone)
{
    Recorder canvas;
    auto state = stateOf(2, 1);

    state.report->reachable.clear();

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_TRUE(canvas.rects.empty());
}

TEST(OverlayDrawTest, DrawValidatorOverlay_OutlinesTheCellOfEveryFinding)
{
    Recorder canvas;
    auto state = stateOf(1, 1);

    state.report->findings.push_back(
        Finding{
            .message = "unreachable trigger",
            .at = cellAt(0, 0),
            .level = 1});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_EQ(
        canvas.lines,
        outlineOf(rectAt(0.0F, 2.0F, 16.0F, 16.0F), kFinding));
}

TEST(OverlayDrawTest, DrawValidatorOverlay_SkipsAFindingThatNamesNoCell)
{
    Recorder canvas;
    auto state = stateOf(1, 1);

    state.report->findings.push_back(
        Finding{.message = "the map has no entry"});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_TRUE(canvas.lines.empty());
}

TEST(
    OverlayDrawTest,
    DrawValidatorOverlay_PutsAFindingWithoutALevelOnTheGround)
{
    Recorder canvas;
    auto state = stateOf(1, 1);

    state.report->findings.push_back(
        Finding{.message = "stray npc", .at = cellAt(0, 0)});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    EXPECT_EQ(
        canvas.lines,
        outlineOf(rectAt(0.0F, 10.0F, 16.0F, 16.0F), kFinding));
}

TEST(OverlayDrawTest, DrawValidatorOverlay_ShadesTheHighestWalkableSurface)
{
    Recorder canvas;
    const auto state = strandedColumnOf(
        {Slab{.level = 0}, Slab{.level = 3}});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(
        canvas.rects[0].rect, rectAt(0.0F, -14.0F, 16.0F, 16.0F));
}

TEST(OverlayDrawTest, DrawValidatorOverlay_LooksBelowAWallForTheSurface)
{
    Recorder canvas;
    const auto state = strandedColumnOf(
        {Slab{.level = 0},
         Slab{.level = 3, .terrain = TerrainClass::Wall}});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(
        canvas.rects[0].rect, rectAt(0.0F, 10.0F, 16.0F, 16.0F));
}

TEST(
    OverlayDrawTest,
    DrawValidatorOverlay_LooksBelowUnbridgedWaterForTheSurface)
{
    Recorder canvas;
    const auto state = strandedColumnOf(
        {Slab{.level = 0},
         Slab{.level = 3, .terrain = TerrainClass::Water}});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(
        canvas.rects[0].rect, rectAt(0.0F, 10.0F, 16.0F, 16.0F));
}

TEST(OverlayDrawTest, DrawValidatorOverlay_StandsOnWaterCarryingABridge)
{
    Recorder canvas;
    const auto state = strandedColumnOf(
        {Slab{.level = 0},
         Slab{
             .level = 3,
             .terrain = TerrainClass::Water,
             .overlay = Overlay::Bridge}});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(
        canvas.rects[0].rect, rectAt(0.0F, -14.0F, 16.0F, 16.0F));
}

TEST(
    OverlayDrawTest,
    DrawValidatorOverlay_FallsBackToTheTopWhenNothingIsWalkable)
{
    Recorder canvas;
    const auto state = strandedColumnOf(
        {Slab{.level = 0},
         Slab{.level = 1, .terrain = TerrainClass::Wall}});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(canvas.rects[0].rect, rectAt(0.0F, 2.0F, 16.0F, 16.0F));
}

TEST(OverlayDrawTest, DrawValidatorOverlay_ShadesAnEmptyColumnOnTheGround)
{
    Recorder canvas;
    const auto state = strandedColumnOf({});

    drawValidatorOverlay(canvas.view(), state, MapCamera{});

    ASSERT_EQ(canvas.rects.size(), 1U);
    EXPECT_EQ(
        canvas.rects[0].rect, rectAt(0.0F, 10.0F, 16.0F, 16.0F));
}

TEST(OverlayDrawTest, DrawMapSelectionOverlay_DrawsNothingWithoutASelection)
{
    Recorder canvas;
    const auto store = storeOf();

    drawMapSelectionOverlay(canvas.view(), store);

    EXPECT_TRUE(canvas.rects.empty());
    EXPECT_TRUE(canvas.lines.empty());
}

TEST(OverlayDrawTest, DrawMapSelectionOverlay_DashesTheMarqueeWhileDragging)
{
    Recorder canvas;
    auto store = storeOf();

    store.mapSelection.dragging = true;
    store.mapSelection.anchor = cellAt(0, 0);
    store.mapSelection.focus = cellAt(1, 1);

    drawMapSelectionOverlay(canvas.view(), store);

    EXPECT_EQ(canvas.rects.size(), 24U);
    EXPECT_THAT(
        canvas.rects,
        Contains(
            DrawnRect{rectAt(0.0F, 10.0F, 3.0F, 1.0F), kWhite}));
    EXPECT_THAT(
        canvas.rects,
        Contains(
            DrawnRect{rectAt(30.0F, 41.0F, 2.0F, 1.0F), kWhite}));
    EXPECT_THAT(
        canvas.rects,
        Contains(
            DrawnRect{rectAt(0.0F, 10.0F, 1.0F, 3.0F), kWhite}));
    EXPECT_THAT(
        canvas.rects,
        Contains(
            DrawnRect{rectAt(31.0F, 40.0F, 1.0F, 2.0F), kWhite}));
    EXPECT_TRUE(canvas.lines.empty());
}

TEST(OverlayDrawTest, DrawMapSelectionOverlay_OutlinesAPlacedSelectionSolid)
{
    Recorder canvas;
    auto store = storeOf();

    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 2, .rows = 1};

    drawMapSelectionOverlay(canvas.view(), store);

    EXPECT_EQ(
        canvas.lines,
        outlineOf(rectAt(16.0F, 26.0F, 32.0F, 16.0F), kWhite));
    EXPECT_TRUE(canvas.rects.empty());
}

TEST(
    OverlayDrawTest,
    DrawMapSelectionOverlay_DisplacesTheOutlineByTheMoveDelta)
{
    Recorder canvas;
    auto store = storeOf();

    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};
    store.mapSelection.moving = true;
    store.mapSelection.moveAnchor = cellAt(1, 1);
    store.mapSelection.movePointer = cellAt(3, 2);

    drawMapSelectionOverlay(canvas.view(), store);

    EXPECT_EQ(canvas.rects.size(), 12U);
    EXPECT_THAT(
        canvas.rects,
        Contains(
            DrawnRect{rectAt(48.0F, 42.0F, 3.0F, 1.0F), kWhite}));
    EXPECT_THAT(
        canvas.rects,
        Contains(
            DrawnRect{rectAt(63.0F, 54.0F, 1.0F, 3.0F), kWhite}));
}

TEST(OverlayDrawTest, DrawMapSelectionOverlay_LiftsTheOutlineByTheActiveLevel)
{
    Recorder canvas;
    auto store = storeOf();

    store.state.activeLevel = 2;
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1};

    drawMapSelectionOverlay(canvas.view(), store);

    EXPECT_EQ(
        canvas.lines,
        outlineOf(rectAt(0.0F, -6.0F, 16.0F, 16.0F), kWhite));
}

TEST(OverlayDrawTest, DrawActiveLevelMarks_DrawsNothingOnTheGroundLevel)
{
    Recorder canvas;
    const auto state = stateOf(2, 1);

    drawActiveLevelMarks(canvas.view(), state, MapCamera{});

    EXPECT_TRUE(canvas.lines.empty());
    EXPECT_TRUE(canvas.rects.empty());
}

TEST(
    OverlayDrawTest,
    DrawActiveLevelMarks_OutlinesEveryColumnHoldingTheActiveLevel)
{
    Recorder canvas;
    auto state = stateOf(2, 1);

    state.activeLevel = 2;
    static_cast<void>(
        state.map.at(cellAt(1, 0)).place(Slab{.level = 2}));

    drawActiveLevelMarks(canvas.view(), state, MapCamera{});

    EXPECT_EQ(
        canvas.lines,
        outlineOf(
            rectAt(16.0F, -6.0F, 16.0F, 16.0F),
            chromeFor(state.map.header().paper).freeMark));
}
