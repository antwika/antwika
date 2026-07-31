#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include <optional>

#include <antwika/input/PointerHint.hpp>

#include "WidgetPixel.hpp"

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/WorldMapState.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/UiSink.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::tests::widgetCentre;
using antwika::game::BuildGhost;
using antwika::game::ghostFor;
using antwika::game::Building;
using antwika::game::BuildingView;
using antwika::game::buildingIndex;
using antwika::game::buildingTile;
using antwika::game::BuildTool;
using antwika::game::buildToolIndex;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::cellCentre;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::GridSink;
using antwika::game::WorldMap;
using antwika::game::WorldMapState;
using antwika::game::InputFold;
using antwika::game::kBuildToolCount;
using antwika::game::Path;
using antwika::game::PathIndex;
using antwika::game::placesBuilding;
using antwika::game::roadTile;
using antwika::game::SceneSnapshot;
using antwika::game::snapshotOf;
using antwika::game::Toolbar;
using antwika::game::toolLabel;
using antwika::game::toolTile;
using antwika::game::UiOverlay;
using antwika::game::UiSink;
using antwika::gfx::Color;
using antwika::gfx::ITexture;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::Position;
using antwika::log::mocks::MockLogger;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
using ::testing::_;
using ::testing::NiceMock;
namespace widgets = antwika::game::widgets;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};
    constexpr GridExtent kExtent{.width = 16, .height = 16};

    constexpr Color kGhostly{
        .red = 255, .green = 255, .blue = 255, .alpha = 110};

    // Every tool but the first, which is the road.
    constexpr std::array<BuildTool, 3> kBuildings{
        BuildTool::House, BuildTool::Shop, BuildTool::Tower};
} // namespace

TEST(BuildToolTest, EveryToolHasItsOwnIndex)
{
    EXPECT_EQ(kBuildToolCount, 4U);
    EXPECT_EQ(buildToolIndex(BuildTool::Road), 0U);
    EXPECT_EQ(buildToolIndex(BuildTool::House), 1U);
    EXPECT_EQ(buildToolIndex(BuildTool::Shop), 2U);
    EXPECT_EQ(buildToolIndex(BuildTool::Tower), 3U);
}

TEST(BuildToolTest, EveryToolButTheRoadPlacesABuilding)
{
    EXPECT_FALSE(placesBuilding(BuildTool::Road));

    for (const auto tool : kBuildings)
    {
        EXPECT_TRUE(placesBuilding(tool));
    }
}

TEST(BuildToolTest, TheBuildingsAreNumberedFromZeroAfterTheRoad)
{
    EXPECT_EQ(buildingIndex(BuildTool::House), 0U);
    EXPECT_EQ(buildingIndex(BuildTool::Shop), 1U);
    EXPECT_EQ(buildingIndex(BuildTool::Tower), 2U);

    // The road places none, and gets an index anyway.
    EXPECT_EQ(buildingIndex(BuildTool::Road), 0U);
}

TEST(BuildPaletteAtlasTest, EveryBuildingHasATileOfItsOwn)
{
    std::vector<Rect> tiles;

    for (const auto tool : kBuildings)
    {
        tiles.push_back(buildingTile(tool));
    }

    EXPECT_NE(tiles[0], tiles[1]);
    EXPECT_NE(tiles[1], tiles[2]);
    EXPECT_NE(tiles[0], tiles[2]);
}

TEST(BuildPaletteAtlasTest, ToolTileIsTheRoadForARoadAndTheBuildingElse)
{
    constexpr std::uint8_t links = 0b0101;

    EXPECT_EQ(toolTile(BuildTool::Road, links), roadTile(links));

    for (const auto tool : kBuildings)
    {
        EXPECT_EQ(toolTile(tool, links), buildingTile(tool));
    }
}

TEST(BuildPaletteWidgetTest, EveryToolIsNamedAndHasItsOwnButton)
{
    EXPECT_EQ(toolLabel(BuildTool::Road), "road");
    EXPECT_EQ(toolLabel(BuildTool::House), "house");
    EXPECT_EQ(toolLabel(BuildTool::Shop), "shop");
    EXPECT_EQ(toolLabel(BuildTool::Tower), "tower");

    EXPECT_NE(widgets::toolWidget(BuildTool::Road), widgets::kZoomIn);
    EXPECT_NE(
        widgets::toolWidget(BuildTool::Road),
        widgets::toolWidget(BuildTool::Tower));
}

TEST(UiOverlayToolTest, TheRoadIsSelectedUntilSomethingSaysOtherwise)
{
    UiOverlay overlay;

    EXPECT_EQ(overlay.tool(), BuildTool::Road);

    overlay.select(BuildTool::Shop);

    EXPECT_EQ(overlay.tool(), BuildTool::Shop);
}

TEST(ToolbarPaletteTest, TheSelectedButtonIsDrawnDifferently)
{
    const Toolbar toolbar;
    const Camera camera;

    const auto road =
        toolbar.describe(kCanvas, Pointer{}, camera, BuildTool::Road);
    const auto tower =
        toolbar.describe(kCanvas, Pointer{}, camera, BuildTool::Tower);

    // The same layout, so only the appearances can have moved.
    EXPECT_EQ(road.commands.size(), tower.commands.size());
    EXPECT_NE(road.commands, tower.commands);
}

namespace
{
    class PaletteSinkTest : public ::testing::Test
    {
    protected:
        // Where a button is, is the layout's business.
        // So a test asks the layout for the one it means.
        [[nodiscard]] Position pixelOn(WidgetId id) const
        {
            const auto centre = widgetCentre(
                toolbar.describe(
                    kCanvas, Pointer{}, camera, overlay.tool()),
                id);

            if (!centre.has_value())
            {
                return Position{};
            }

            return Position{.x = centre->x, .y = centre->y};
        }

        // The fold first, then the UI, then the grid.
        // That is the order bootstrap() registers them in.
        void send(const InputEvent &event)
        {
            const TickEvent wrapped{
                .tick = 0, .event = codec.encode(event)};

            input.handle(wrapped);
            uiSink.handle(wrapped);
            gridSink.handle(wrapped);
        }

        void tick()
        {
            const TickEvent wrapped{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick}};

            input.handle(wrapped);
            uiSink.handle(wrapped);
            gridSink.handle(wrapped);
        }

        [[nodiscard]] Position pixelOf(Cell cell) const
        {
            const auto point = cellCentre(cell, camera);
            return Position{.x = point.x, .y = point.y};
        }

        void pressOn(WidgetId id)
        {
            send(
                PointerButtonPressed{
                    .button = MouseButton::Left, .position = pixelOn(id)});
        }

        void clickAt(Cell cell, MouseButton button)
        {
            send(
                PointerButtonPressed{
                    .button = button, .position = pixelOf(cell)});
        }

        [[nodiscard]] std::vector<BuildingView> buildings()
        {
            world.commit();

            return snapshotOf(world, paths, camera, kExtent).buildings;
        }

        NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;

        // Panned well clear of the bar, which sits top-left.
        Camera camera{Point{.x = 700, .y = 300}};
        SystemScheduler scheduler;
        InputEventCodec codec;
        InputFold input{codec};
        UiOverlay overlay{kCanvas};
        Toolbar toolbar;
        UiSink uiSink{camera, overlay, input, toolbar, camera};
        WorldMapState cities{WorldMap{}};
        GridSink gridSink{
            world,
            paths,
            camera,
            kExtent,
            scheduler,
            input,
            overlay,
            cities};
    };
} // namespace

TEST_F(PaletteSinkTest, PressingAPaletteButtonSelectsThatTool)
{
    for (const auto tool : {
             BuildTool::House,
             BuildTool::Shop,
             BuildTool::Tower,
             BuildTool::Road,
         })
    {
        pressOn(widgets::toolWidget(tool));

        EXPECT_EQ(overlay.tool(), tool);
    }
}

TEST_F(PaletteSinkTest, PressingSomethingElseLeavesTheToolAlone)
{
    pressOn(widgets::toolWidget(BuildTool::Shop));

    // The bar is under this one, so it selects nothing.
    pressOn(widgets::kZoomIn);

    // And this one is on the grid, so it activates no widget at all.
    clickAt(Cell{.x = 4, .y = 4}, MouseButton::Left);

    EXPECT_EQ(overlay.tool(), BuildTool::Shop);
}

TEST_F(PaletteSinkTest, AClickOnTheBarPlacesNothingOnTheGrid)
{
    pressOn(widgets::toolWidget(BuildTool::House));

    EXPECT_TRUE(buildings().empty());
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(PaletteSinkTest, TheSelectedToolIsWhatALeftClickPlaces)
{
    constexpr Cell target{.x = 3, .y = 4};

    pressOn(widgets::toolWidget(BuildTool::Tower));
    clickAt(target, MouseButton::Left);

    const auto placed = buildings();

    ASSERT_EQ(placed.size(), 1U);
    EXPECT_EQ(placed[0].at, target);
    EXPECT_EQ(placed[0].kind, BuildTool::Tower);
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(PaletteSinkTest, TheRoadStaysWhatALeftClickPlacesByDefault)
{
    constexpr Cell target{.x = 5, .y = 5};

    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_TRUE(buildings().empty());
}

TEST_F(PaletteSinkTest, ACellTakesOneThingOnly)
{
    constexpr Cell target{.x = 2, .y = 6};

    pressOn(widgets::toolWidget(BuildTool::House));
    clickAt(target, MouseButton::Left);
    clickAt(target, MouseButton::Left);

    // A second building, and then a road, are both refused.
    pressOn(widgets::toolWidget(BuildTool::Road));
    clickAt(target, MouseButton::Left);

    EXPECT_EQ(buildings().size(), 1U);
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(PaletteSinkTest, ABuildingRefusesACellAlreadyPaved)
{
    constexpr Cell target{.x = 7, .y = 1};

    clickAt(target, MouseButton::Left);

    pressOn(widgets::toolWidget(BuildTool::Shop));
    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_TRUE(buildings().empty());
}

TEST_F(PaletteSinkTest, ABuildingGoesNowhereOutsideTheExtent)
{
    pressOn(widgets::toolWidget(BuildTool::Shop));

    for (const auto outside : {
             Cell{.x = -1, .y = 0},
             Cell{.x = kExtent.width, .y = 0},
         })
    {
        clickAt(outside, MouseButton::Left);
    }

    EXPECT_TRUE(buildings().empty());
}

// The ghost is worked out on the render side, from an unrecorded hint.
// So it is a pure function rather than anything a sink stages.
namespace
{
    [[nodiscard]] antwika::input::PointerHint hintOn(
        Cell cell, const Camera &camera)
    {
        const auto point = cellCentre(cell, camera);
        return antwika::input::PointerHint{
            .position = {.x = point.x, .y = point.y}};
    }
} // namespace

TEST(BuildGhostTest, GhostFor_FollowsThePointerAndTheSelectedTool)
{
    constexpr Cell target{.x = 6, .y = 3};
    const Camera camera;

    const auto shown = ghostFor(
        hintOn(target, camera),
        camera,
        kExtent,
        BuildTool::Tower,
        false);

    EXPECT_TRUE(shown.visible);
    EXPECT_EQ(shown.at, target);
    EXPECT_EQ(shown.tool, BuildTool::Tower);
}

TEST(BuildGhostTest, GhostFor_IsInvisibleUntilSomethingLocatesThePointer)
{
    const auto shown = ghostFor(
        std::nullopt, Camera(), kExtent, BuildTool::Road, false);

    EXPECT_FALSE(shown.visible);
    EXPECT_EQ(shown.tool, BuildTool::Road);
}

TEST(BuildGhostTest, GhostFor_IsInvisibleOffTheGrid)
{
    const Camera camera;

    EXPECT_FALSE(
        ghostFor(
            hintOn(Cell{.x = -3, .y = -3}, camera),
            camera,
            kExtent,
            BuildTool::Road,
            false)
            .visible);
}

// What the bar covers, it covers from the ghost too.
// The answer comes *from* UiOverlay, never the other way round.
TEST(BuildGhostTest, GhostFor_IsInvisibleUnderTheToolbar)
{
    const Camera camera;

    EXPECT_FALSE(
        ghostFor(
            hintOn(Cell{.x = 1, .y = 1}, camera),
            camera,
            kExtent,
            BuildTool::Road,
            true)
            .visible);
}

// Reading simulation state in order to draw is fine.
// Which cell a pixel means is a function of the camera.
TEST(BuildGhostTest, GhostFor_ResolvesThroughTheCameraItIsGiven)
{
    const Camera panned(antwika::gfx::Point{.x = 200, .y = 30});

    const auto shown = ghostFor(
        hintOn(Cell{.x = 2, .y = 2}, panned),
        panned,
        kExtent,
        BuildTool::House,
        false);

    EXPECT_TRUE(shown.visible);
    EXPECT_EQ(shown.at, (Cell{.x = 2, .y = 2}));
}

TEST(SceneSnapshotBuildTest, AnUntouchedWorldHasNoBuildingsAndNoGhost)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    EXPECT_TRUE(snapshot.buildings.empty());
    EXPECT_FALSE(snapshot.ghost.visible);
}

TEST(SceneSnapshotBuildTest, SnapshotOf_TakesTheBuildingsAndNoGhost)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const PathIndex paths;

    const auto shop = world.create();
    world.add<Cell>(shop, Cell{.x = 2, .y = 3});
    world.add<Building>(shop, Building{.kind = BuildTool::Shop});

    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.buildings.size(), 1U);
    EXPECT_EQ(snapshot.buildings[0].at, (Cell{.x = 2, .y = 3}));
    EXPECT_EQ(snapshot.buildings[0].kind, BuildTool::Shop);

    // Whoever draws fills this in, from a channel no replay holds.
    EXPECT_FALSE(snapshot.ghost.visible);
}

namespace
{
    struct Blit
    {
        Rect source;
        Rect destination;
        Color tint;
    };

    // The one cell the camera puts anywhere near the canvas.
    // A snapshot with no extent then draws nothing but what a test adds.
    [[nodiscard]] SceneSnapshot emptySnapshot()
    {
        return SceneSnapshot{
            .camera = Camera(),
            .extent = GridExtent{.width = 0, .height = 0},
            .paths = {},
            .walkers = {},
            .buildings = {},
            .ghost = {}};
    }

    [[nodiscard]] std::vector<Blit> blitsOf(const SceneSnapshot &snapshot)
    {
        std::vector<Blit> blits;
        NiceMock<MockRenderer> renderer;
        const NiceMock<MockTexture> atlas;

        ON_CALL(renderer, drawTexture(_, _, _, _))
            .WillByDefault(
                [&blits](
                    const ITexture &, Rect source, Rect destination,
                    Color tint)
                { blits.push_back(Blit{source, destination, tint}); });

        const GridScene scene;
        scene.draw(renderer, kCanvas, snapshot, atlas);

        return blits;
    }
} // namespace

TEST(GridSceneBuildTest, ABuildingIsOneBlitOfItsOwnTile)
{
    auto snapshot = emptySnapshot();
    snapshot.buildings.push_back(
        BuildingView{.at = Cell{.x = 0, .y = 0}, .kind = BuildTool::Shop});

    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(blits[0].source, buildingTile(BuildTool::Shop));
    EXPECT_EQ(
        blits[0].destination,
        cellBounds(Cell{.x = 0, .y = 0}, snapshot.camera));
}

TEST(GridSceneBuildTest, ABuildingOffTheCanvasIsNotDrawn)
{
    auto snapshot = emptySnapshot();
    snapshot.buildings.push_back(
        BuildingView{
            .at = Cell{.x = 900, .y = -900}, .kind = BuildTool::House});

    EXPECT_TRUE(blitsOf(snapshot).empty());
}

TEST(GridSceneBuildTest, TheGhostIsDrawnLastAndSeeThrough)
{
    auto snapshot = emptySnapshot();
    snapshot.buildings.push_back(
        BuildingView{.at = Cell{.x = 0, .y = 0}, .kind = BuildTool::House});
    snapshot.ghost = BuildGhost{
        .at = Cell{.x = 0, .y = 0},
        .tool = BuildTool::Tower,
        .visible = true};

    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 2U);
    EXPECT_EQ(blits[1].source, buildingTile(BuildTool::Tower));
    EXPECT_EQ(blits[1].tint, kGhostly);
}

TEST(GridSceneBuildTest, ARoadGhostShowsTheJunctionItWouldBecome)
{
    auto snapshot = emptySnapshot();
    snapshot.paths.push_back(Cell{.x = 0, .y = 1});
    snapshot.ghost = BuildGhost{
        .at = Cell{.x = 0, .y = 0},
        .tool = BuildTool::Road,
        .visible = true};

    const auto blits = blitsOf(snapshot);

    // The road already there, then the ghost joining it southward.
    ASSERT_EQ(blits.size(), 2U);
    EXPECT_EQ(
        blits[1].source,
        roadTile(antwika::game::linkBit(antwika::game::Direction::South)));
    EXPECT_EQ(blits[1].tint, kGhostly);
}

TEST(GridSceneBuildTest, AnInvisibleOrOffscreenGhostIsNotDrawn)
{
    auto hidden = emptySnapshot();
    hidden.ghost =
        BuildGhost{
            .at = Cell{.x = 0, .y = 0},
            .tool = BuildTool::House,
            .visible = false};

    EXPECT_TRUE(blitsOf(hidden).empty());

    auto away = emptySnapshot();
    away.ghost = BuildGhost{
        .at = Cell{.x = 900, .y = -900},
        .tool = BuildTool::House,
        .visible = true};

    EXPECT_TRUE(blitsOf(away).empty());
}
