#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
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

#include "FakeMenuCommands.hpp"
#include "TestTranslator.hpp"
#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/FootprintOutline.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/GridSink.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/MenuModalScene.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/UiSink.hpp"
#include "antwika/game/ViewCommands.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::game::GameConfig;
using antwika::game::ViewCommands;
using antwika::game::tests::kTranslator;

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::BuildGhost;
using antwika::game::Building;
using antwika::game::BuildingIndex;
using antwika::game::BuildingKind;
using antwika::game::buildingKindIndex;
using antwika::game::BuildingSprite;
using antwika::game::buildingTile;
using antwika::game::BuildTool;
using antwika::game::buildToolIndex;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::cellCentre;
using antwika::game::footprintBounds;
using antwika::game::footprintOf;
using antwika::game::footprintOutline;
using antwika::game::ghostFor;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::GridSink;
using antwika::game::InputFold;
using antwika::game::kBuildToolCount;
using antwika::game::kOutlineCorners;
using antwika::game::MenuModalScene;
using antwika::game::Path;
using antwika::game::PathIndex;
using antwika::game::PauseState;
using antwika::game::placesBuilding;
using antwika::game::RoadDrag;
using antwika::game::roadTile;
using antwika::game::SceneSnapshot;
using antwika::game::snapshotOf;
using antwika::game::tests::widgetCentre;
using antwika::game::Toolbar;
using antwika::game::toolTile;
using antwika::game::UiOverlay;
using antwika::game::UiSink;
using antwika::game::WorldMap;
using antwika::game::WorldMapState;
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
    // Every tool that places a building: all but Road and Raze.
    constexpr std::array<BuildTool, kBuildToolCount - 2> kBuildings{
        BuildTool::House,
        BuildTool::Farm,
        BuildTool::ClayPit,
        BuildTool::Workshop,
        BuildTool::Storage,
        BuildTool::Market,
        BuildTool::Well,
        BuildTool::Doctor,
        BuildTool::FireStation,
        BuildTool::EngineerPost};
} // namespace

TEST(BuildToolTest, EveryToolHasItsOwnIndex)
{
    EXPECT_EQ(kBuildToolCount, 12U);
    EXPECT_EQ(buildToolIndex(BuildTool::Road), 0U);
    EXPECT_EQ(buildToolIndex(BuildTool::House), 1U);
    EXPECT_EQ(buildToolIndex(BuildTool::Farm), 2U);
    EXPECT_EQ(buildToolIndex(BuildTool::EngineerPost), 10U);
    EXPECT_EQ(buildToolIndex(BuildTool::Raze), 11U);
}

TEST(BuildToolTest, EveryToolButRoadAndRazePlacesABuilding)
{
    EXPECT_FALSE(placesBuilding(BuildTool::Road));
    EXPECT_FALSE(placesBuilding(BuildTool::Raze));

    for (const auto tool : kBuildings)
    {
        EXPECT_TRUE(placesBuilding(tool));
    }
}

TEST(BuildToolTest, EveryToolButTheRoadNamesABuildingKind)
{
    EXPECT_EQ(buildingKindOf(BuildTool::House), BuildingKind::House);
    EXPECT_EQ(
        buildingKindOf(BuildTool::Farm), BuildingKind::Farm);
    EXPECT_EQ(
        buildingKindOf(BuildTool::EngineerPost),
        BuildingKind::EngineerPost);

    // The road places none, and says so rather than naming a kind.
    EXPECT_FALSE(buildingKindOf(BuildTool::Road).has_value());
}

TEST(BuildPaletteAtlasTest, EveryBuildingHasATileOfItsOwn)
{
    std::vector<Rect> tiles;

    for (const auto tool : kBuildings)
    {
        tiles.push_back(buildingTile(*buildingKindOf(tool)));
    }

    for (std::size_t i = 0; i < tiles.size(); ++i)
    {
        for (std::size_t j = i + 1; j < tiles.size(); ++j)
        {
            EXPECT_NE(tiles[i], tiles[j]) << i << " vs " << j;
        }
    }
}

TEST(BuildPaletteAtlasTest, ToolTileIsTheRoadForARoadAndTheBuildingElse)
{
    constexpr std::uint8_t links = 0b0101;

    EXPECT_EQ(toolTile(BuildTool::Road, links), roadTile(links));

    for (const auto tool : kBuildings)
    {
        EXPECT_EQ(
            toolTile(tool, links), buildingTile(*buildingKindOf(tool)));
    }
}

// What each tool is *called* is ToolLabelTest's subject.
// This one is about a tool having a button nothing else shares.
TEST(BuildPaletteWidgetTest, EveryToolHasAButtonOfItsOwn)
{
    std::set<antwika::ui::WidgetId> ids{
        widgets::kZoomIn,
        widgets::kZoomOut,
        widgets::kResetView,
        widgets::kPauseResume,
        widgets::kMenu};

    for (std::size_t index = 0; index < kBuildToolCount; ++index)
    {
        ids.insert(widgets::toolWidget(static_cast<BuildTool>(index)));
    }

    EXPECT_EQ(ids.size(), kBuildToolCount + 5U);
}

TEST(UiOverlayToolTest, TheRoadIsSelectedUntilSomethingSaysOtherwise)
{
    UiOverlay overlay;

    EXPECT_EQ(overlay.tool(), BuildTool::Road);

    overlay.select(BuildTool::Farm);

    EXPECT_EQ(overlay.tool(), BuildTool::Farm);
}

// Putting the palette down is a state, not a tool.
// So it is asked about with has_value() rather than compared to one.
TEST(UiOverlayToolTest, ThePaletteCanBePutDownAndPickedBackUp)
{
    UiOverlay overlay;

    overlay.select(BuildTool::House);
    overlay.clearTool();

    EXPECT_FALSE(overlay.tool().has_value());

    overlay.select(BuildTool::House);

    EXPECT_EQ(overlay.tool(), BuildTool::House);
}

TEST(ToolbarPaletteTest, TheSelectedButtonIsDrawnDifferently)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;

    const auto road =
        toolbar.describe(kCanvas, Pointer{}, camera, BuildTool::Road);
    const auto tower =
        toolbar.describe(kCanvas, Pointer{}, camera, BuildTool::Well);

    // The same layout, so only the appearances can have moved.
    EXPECT_EQ(road.commands.size(), tower.commands.size());
    EXPECT_NE(road.commands, tower.commands);
}

// A bar holding a tool down while nothing is selected would lie.
// It would say a left click places something, where it places nothing.
TEST(ToolbarPaletteTest, NoButtonIsHeldDownWithThePalettePutDown)
{
    const Toolbar toolbar{kTranslator};
    const Camera camera;

    const auto down =
        toolbar.describe(kCanvas, Pointer{}, camera, std::nullopt);

    for (std::size_t index = 0; index < kBuildToolCount; ++index)
    {
        const auto tool = static_cast<BuildTool>(index);
        const auto held =
            toolbar.describe(kCanvas, Pointer{}, camera, tool);

        // Same layout throughout, so a difference is an appearance.
        EXPECT_EQ(down.commands.size(), held.commands.size());
        EXPECT_NE(down.commands, held.commands);
    }
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

        [[nodiscard]] std::vector<BuildingSprite> buildings()
        {
            world.commit();

            return snapshotOf(world, paths, camera, kExtent).buildings;
        }

        NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;

        // Panned so the cells these tests click sit in the middle.
        // Clear of the two strips, and clear of the palette panel.
        Camera camera{Point{.x = 400, .y = 200}};
        SystemScheduler scheduler;
        InputEventCodec codec;
        InputFold input{codec};
        UiOverlay overlay{kCanvas};
        Toolbar toolbar{kTranslator};
        PauseState pause;
        antwika::game::MapViewState mapView;
        WorldMapState cities{WorldMap{}};
        RoadDrag drag;
        antwika::game::tests::FakeMenuCommands commands;
        MenuModalScene modalScene{kTranslator};
        antwika::game::CityRatings ratings;
        antwika::game::GameState state;
        ViewCommands viewCommands{camera, pause, camera};
        UiSink uiSink{
            camera,
            overlay,
            input,
            toolbar,
            pause,
            mapView,
            commands,
            drag,
            modalScene,
            viewCommands,
            ratings,
            state};
        GridSink gridSink{
            world,
            paths,
            camera,
            kExtent,
            scheduler,
            input,
            overlay,
            cities,
            built,
            drag,
            state,
            GameConfig{}};
    };
} // namespace

TEST_F(PaletteSinkTest, PressingAPaletteButtonSelectsThatTool)
{
    for (const auto tool : {
             BuildTool::House,
             BuildTool::Farm,
             BuildTool::Well,
             BuildTool::Road,
         })
    {
        pressOn(widgets::toolWidget(tool));

        EXPECT_EQ(overlay.tool(), tool);
    }
}

TEST_F(PaletteSinkTest, PressingSomethingElseLeavesTheToolAlone)
{
    pressOn(widgets::toolWidget(BuildTool::Farm));

    // The bar is under this one, so it selects nothing.
    pressOn(widgets::kZoomIn);

    // And this one is on the grid, so it activates no widget at all.
    clickAt(Cell{.x = 4, .y = 4}, MouseButton::Left);

    EXPECT_EQ(overlay.tool(), BuildTool::Farm);
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

    pressOn(widgets::toolWidget(BuildTool::Well));
    clickAt(target, MouseButton::Left);

    const auto placed = buildings();

    ASSERT_EQ(placed.size(), 1U);
    EXPECT_EQ(placed[0].at, target);
    EXPECT_EQ(placed[0].kind, BuildingKind::Well);
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(PaletteSinkTest, TheRoadStaysWhatALeftClickPlacesByDefault)
{
    constexpr Cell target{.x = 5, .y = 5};

    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_TRUE(buildings().empty());
}

// The rule a right press follows, through the real bar and grid.
// Selected with a button, cancelled with a click, palette put down.
TEST_F(PaletteSinkTest, ARightClickLeavesBuildModeAndSelectsNothing)
{
    constexpr Cell target{.x = 3, .y = 4};

    pressOn(widgets::toolWidget(BuildTool::House));
    clickAt(target, MouseButton::Right);

    EXPECT_FALSE(overlay.tool().has_value());

    // So the next left click lays nothing at all, road included.
    clickAt(target, MouseButton::Left);

    EXPECT_FALSE(paths.has(target));
    EXPECT_TRUE(buildings().empty());
}

// The palette is picked back up by pressing a button, and only that.
// Which is what makes putting it down a state rather than a dead end.
TEST_F(PaletteSinkTest, APaletteButtonSelectsAgainAfterACancel)
{
    constexpr Cell target{.x = 3, .y = 4};

    pressOn(widgets::toolWidget(BuildTool::House));
    clickAt(target, MouseButton::Right);
    pressOn(widgets::toolWidget(BuildTool::Road));
    clickAt(target, MouseButton::Left);

    EXPECT_EQ(overlay.tool(), BuildTool::Road);
    EXPECT_TRUE(paths.has(target));
}

// The bar is described again after a cancel, so it shows the change.
// Otherwise the house button would look held down until the next tick.
TEST_F(PaletteSinkTest, ARightClickIsShownOnTheBarStraightAway)
{
    pressOn(widgets::toolWidget(BuildTool::House));
    clickAt(Cell{.x = 3, .y = 4}, MouseButton::Right);
    tick();

    const auto down =
        toolbar.describe(kCanvas, Pointer{}, camera, std::nullopt);

    EXPECT_EQ(overlay.commands(), down.commands);

    // And that picture is not the one a selected tool draws.
    const auto road =
        toolbar.describe(kCanvas, Pointer{}, camera, BuildTool::Road);

    EXPECT_NE(overlay.commands(), road.commands);
}

// A block holds every cell, not only the one clicked.
// Asserted through the real sink rather than through the index.
TEST_F(PaletteSinkTest, ABlockHoldsEveryCellItCovers)
{
    constexpr Cell target{.x = 2, .y = 6};

    pressOn(widgets::toolWidget(BuildTool::Farm));
    clickAt(target, MouseButton::Left);

    ASSERT_EQ(buildings().size(), 1U);

    // A road on the block's far corner is refused all the same.
    pressOn(widgets::toolWidget(BuildTool::Road));
    clickAt(Cell{.x = 3, .y = 7}, MouseButton::Left);

    EXPECT_EQ(paths.size(), 0U);

    // And one just past it is not.
    clickAt(Cell{.x = 4, .y = 7}, MouseButton::Left);

    EXPECT_EQ(paths.size(), 1U);
}

TEST_F(PaletteSinkTest, ABlockRefusesToOverlapAnother)
{
    pressOn(widgets::toolWidget(BuildTool::Farm));
    clickAt(Cell{.x = 2, .y = 6}, MouseButton::Left);

    // Overlapping at one corner only, which is still overlapping.
    clickAt(Cell{.x = 3, .y = 7}, MouseButton::Left);

    EXPECT_EQ(buildings().size(), 1U);
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

    pressOn(widgets::toolWidget(BuildTool::Farm));
    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_TRUE(buildings().empty());
}

TEST_F(PaletteSinkTest, ABuildingGoesNowhereOutsideTheExtent)
{
    pressOn(widgets::toolWidget(BuildTool::Farm));

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
    // Nothing laid and nothing built.
    // So these tests are about where a ghost goes.
    const PathIndex kNoPaths;
    const BuildingIndex kNothingBuilt;

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
        BuildTool::Well,
        false,
        kNoPaths,
        kNothingBuilt);

    EXPECT_TRUE(shown.visible);
    EXPECT_EQ(shown.at, target);
    EXPECT_EQ(shown.tool, BuildTool::Well);
}

TEST(BuildGhostTest, GhostFor_IsInvisibleUntilSomethingLocatesThePointer)
{
    const auto shown = ghostFor(
        std::nullopt,
        Camera(),
        kExtent,
        BuildTool::Road,
        false,
        kNoPaths,
        kNothingBuilt);

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
            false,
            kNoPaths,
            kNothingBuilt)
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
            true,
            kNoPaths,
            kNothingBuilt)
            .visible);
}

// Nothing selected places nothing, so it previews nothing.
// A ghost drawn there would promise a click this app would refuse.
TEST(BuildGhostTest, GhostFor_IsInvisibleWithThePalettePutDown)
{
    const Camera camera;

    const auto shown = ghostFor(
        hintOn(Cell{.x = 1, .y = 1}, camera),
        camera,
        kExtent,
        std::nullopt,
        false,
        kNoPaths,
        kNothingBuilt);

    EXPECT_FALSE(shown.visible);
    EXPECT_FALSE(shown.valid);
}

// A razing click places nothing, so there is no block to promise.
// What is standing under the pointer is the hover readout's job.
TEST(BuildGhostTest, GhostFor_IsInvisibleForTheRazeTool)
{
    const Camera camera;

    const auto shown = ghostFor(
        hintOn(Cell{.x = 1, .y = 1}, camera),
        camera,
        kExtent,
        BuildTool::Raze,
        false,
        kNoPaths,
        kNothingBuilt);

    EXPECT_FALSE(shown.visible);
    EXPECT_FALSE(shown.valid);
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
        false,
        kNoPaths,
        kNothingBuilt);

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
    world.add<Building>(shop, Building{.kind = BuildingKind::Farm});

    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.buildings.size(), 1U);
    EXPECT_EQ(snapshot.buildings[0].at, (Cell{.x = 2, .y = 3}));
    EXPECT_EQ(snapshot.buildings[0].kind, BuildingKind::Farm);

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
            .ruins = {},
            .plan = {},
            .ghost = {},
            .hover = {},
            .overlay = {}};
    }

    [[nodiscard]] std::vector<Blit> blitsOf(const SceneSnapshot &snapshot)
    {
        std::vector<Blit> blits;
        NiceMock<MockRenderer> renderer;
        const NiceMock<MockTexture> atlas;
        const antwika::game::AtlasTextures atlases{
            .oneByOne = atlas, .twoByTwo = atlas, .threeByThree = atlas};

        ON_CALL(renderer, drawTexture(_, _, _, _))
            .WillByDefault(
                [&blits](
                    const ITexture &, Rect source, Rect destination,
                    Color tint)
                { blits.push_back(Blit{source, destination, tint}); });

        const GridScene scene{kTranslator};
        scene.draw(renderer, kCanvas, snapshot, atlases);

        return blits;
    }
} // namespace

TEST(GridSceneBuildTest, ABuildingIsOneBlitOfItsOwnTile)
{
    auto snapshot = emptySnapshot();
    snapshot.buildings.push_back(
        BuildingSprite{
            .at = Cell{.x = 0, .y = 0},
            .kind = BuildingKind::Farm});

    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits[0].source, buildingTile(BuildingKind::Farm));
    EXPECT_EQ(
        blits[0].destination,
        antwika::game::buildingSpriteBounds(
            Cell{.x = 0, .y = 0}, BuildingKind::Farm, snapshot.camera));
}

TEST(GridSceneBuildTest, ABuildingOffTheCanvasIsNotDrawn)
{
    auto snapshot = emptySnapshot();
    snapshot.buildings.push_back(
        BuildingSprite{
            .at = Cell{.x = 900, .y = -900},
            .kind = BuildingKind::House});

    EXPECT_TRUE(blitsOf(snapshot).empty());
}

TEST(GridSceneBuildTest, TheGhostIsDrawnLastAndSeeThrough)
{
    auto snapshot = emptySnapshot();
    snapshot.buildings.push_back(
        BuildingSprite{
            .at = Cell{.x = 0, .y = 0}, .kind = BuildingKind::House});
    snapshot.ghost = BuildGhost{
        .at = Cell{.x = 0, .y = 0},
        .tool = BuildTool::Well,
        .visible = true,
        .valid = true};

    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 2U);
    EXPECT_EQ(
        blits[1].source, buildingTile(BuildingKind::Well));
    EXPECT_EQ(blits[1].tint, kGhostly);
}

TEST(GridSceneBuildTest, ARoadGhostShowsTheJunctionItWouldBecome)
{
    auto snapshot = emptySnapshot();
    snapshot.extent = GridExtent{.width = 1, .height = 2};
    snapshot.paths.push_back(Cell{.x = 0, .y = 1});
    snapshot.ghost = BuildGhost{
        .at = Cell{.x = 0, .y = 0},
        .tool = BuildTool::Road,
        .visible = true,
        .valid = true};

    const auto blits = blitsOf(snapshot);

    // Two cells of ground, then the laid road, then the ghost.
    // The ghost joins the laid road southward.
    ASSERT_EQ(blits.size(), 4U);
    EXPECT_EQ(
        blits.back().source,
        roadTile(antwika::game::linkBit(antwika::game::Direction::South)));
    EXPECT_EQ(blits.back().tint, kGhostly);
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

TEST(BuildGhostTest, GhostFor_SaysAClearBlockWouldLand)
{
    constexpr Cell target{.x = 6, .y = 3};
    const Camera camera;

    const auto shown = ghostFor(
        hintOn(target, camera),
        camera,
        kExtent,
        BuildTool::Farm,
        false,
        kNoPaths,
        kNothingBuilt);

    EXPECT_TRUE(shown.visible);
    EXPECT_TRUE(shown.valid);
}

// Shown rather than hidden.
// So a refusal is something to act on rather than a disappearance.
TEST(BuildGhostTest, GhostFor_ShowsABlockedBlockAndSaysItWouldNot)
{
    constexpr Cell target{.x = 6, .y = 3};
    const Camera camera;

    PathIndex paths;
    paths.insert(Cell{.x = 7, .y = 4});

    const auto shown = ghostFor(
        hintOn(target, camera),
        camera,
        kExtent,
        BuildTool::Farm,
        false,
        paths,
        kNothingBuilt);

    EXPECT_TRUE(shown.visible);
    EXPECT_FALSE(shown.valid);
}

TEST(BuildGhostTest, GhostFor_HidesABlockHangingOffTheGrid)
{
    const Camera camera;
    const Cell corner{.x = kExtent.width - 1, .y = kExtent.height - 1};

    const auto shown = ghostFor(
        hintOn(corner, camera),
        camera,
        kExtent,
        BuildTool::Storage,
        false,
        kNoPaths,
        kNothingBuilt);

    EXPECT_FALSE(shown.visible);
}

TEST(BuildGhostTest, GhostEqualityComparesEveryField)
{
    constexpr BuildGhost base{
        .at = Cell{.x = 1, .y = 2},
        .tool = BuildTool::House,
        .visible = true,
        .valid = true};

    EXPECT_EQ(base, base);

    auto moved = base;
    moved.at = Cell{.x = 9, .y = 9};
    EXPECT_NE(base, moved);

    auto other = base;
    other.tool = BuildTool::Road;
    EXPECT_NE(base, other);

    auto hidden = base;
    hidden.visible = false;
    EXPECT_NE(base, hidden);

    auto blocked = base;
    blocked.valid = false;
    EXPECT_NE(base, blocked);
}

TEST(GridSceneBuildTest, ABlockedGhostIsDrawnInADifferentTint)
{
    auto snapshot = emptySnapshot();
    snapshot.ghost = BuildGhost{
        .at = Cell{.x = 0, .y = 0},
        .tool = BuildTool::House,
        .visible = true,
        .valid = false};

    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_NE(blits[0].tint, kGhostly);
}

// The border round the block, which is four lines rather than a fill.
// A diamond's edges are diagonal and drawRect takes an upright box.
namespace
{
    struct Line
    {
        Point from;
        Point to;
        Color color;

        [[nodiscard]] bool operator==(const Line &other) const = default;
    };

    [[nodiscard]] std::vector<Line> linesOf(const SceneSnapshot &snapshot)
    {
        std::vector<Line> lines;
        NiceMock<MockRenderer> renderer;
        const NiceMock<MockTexture> atlas;
        const antwika::game::AtlasTextures atlases{
            .oneByOne = atlas, .twoByTwo = atlas, .threeByThree = atlas};

        ON_CALL(renderer, drawLine(_, _, _))
            .WillByDefault(
                [&lines](Point from, Point to, Color color)
                { lines.push_back(Line{from, to, color}); });

        const GridScene scene{kTranslator};
        scene.draw(renderer, kCanvas, snapshot, atlases);

        return lines;
    }

    // What a border is: every corner joined to the next, and closed.
    [[nodiscard]] std::vector<Line> loopOf(
        const std::array<Point, kOutlineCorners> &corners, Color color)
    {
        std::vector<Line> loop;

        for (std::size_t corner = 0; corner < corners.size(); ++corner)
        {
            loop.push_back(
                Line{
                    corners[corner],
                    corners[(corner + 1) % corners.size()],
                    color});
        }

        return loop;
    }

    [[nodiscard]] SceneSnapshot ghostSnapshot(BuildTool tool, bool valid)
    {
        auto snapshot = emptySnapshot();
        snapshot.ghost = BuildGhost{
            .at = Cell{.x = 0, .y = 0},
            .tool = tool,
            .visible = true,
            .valid = valid};

        return snapshot;
    }
} // namespace

TEST(GridSceneBuildTest, TheGhostIsBorderedRoundItsWholeBlock)
{
    const auto snapshot = ghostSnapshot(BuildTool::Farm, true);
    const auto lines = linesOf(snapshot);

    ASSERT_EQ(lines.size(), kOutlineCorners);
    EXPECT_EQ(
        lines,
        loopOf(
            footprintOutline(
                snapshot.ghost.at,
                footprintOf(BuildingKind::Farm),
                snapshot.camera),
            lines.front().color));
}

// The border is the block the click takes, so a road's is one cell.
TEST(GridSceneBuildTest, ARoadGhostIsBorderedRoundTheOneCell)
{
    const auto snapshot = ghostSnapshot(BuildTool::Road, true);
    const auto lines = linesOf(snapshot);

    EXPECT_EQ(
        lines,
        loopOf(
            footprintOutline(
                snapshot.ghost.at,
                antwika::game::Footprint{},
                snapshot.camera),
            lines.front().color));
}

// The same border the ghost tile is drawn in, taken round the same box.
// Two ways of working that box out is exactly what this rules out.
TEST(GridSceneBuildTest, TheBorderTracesTheBoxTheGhostIsBlittedInto)
{
    const auto snapshot = ghostSnapshot(BuildTool::EngineerPost, true);
    const auto lines = linesOf(snapshot);
    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 1U);
    ASSERT_EQ(lines.size(), kOutlineCorners);

    const auto box = blits.front().destination;
    const auto right =
        box.origin.x + static_cast<std::int32_t>(box.size.width) - 1;
    const auto bottom =
        box.origin.y + static_cast<std::int32_t>(box.size.height) - 1;

    for (const auto &line : lines)
    {
        for (const auto point : {line.from, line.to})
        {
            EXPECT_GE(point.x, box.origin.x);
            EXPECT_LE(point.x, right);
            EXPECT_GE(point.y, box.origin.y);
            EXPECT_LE(point.y, bottom);
        }
    }
}

TEST(GridSceneBuildTest, ABlockedGhostIsBorderedInADifferentColour)
{
    const auto refused = linesOf(ghostSnapshot(BuildTool::House, false));
    const auto allowed = linesOf(ghostSnapshot(BuildTool::House, true));

    ASSERT_EQ(refused.size(), kOutlineCorners);
    ASSERT_EQ(allowed.size(), kOutlineCorners);
    EXPECT_NE(refused.front().color, allowed.front().color);

    // The refusal is opaque, unlike the tile it surrounds.
    // An edge as faint as the placeholder is the one nobody would see.
    EXPECT_GT(refused.front().color.alpha, kGhostly.alpha);
}

TEST(GridSceneBuildTest, NothingIsBorderedWithNoGhostToBorder)
{
    EXPECT_TRUE(linesOf(emptySnapshot()).empty());

    auto away = ghostSnapshot(BuildTool::House, true);
    away.ghost.at = Cell{.x = 900, .y = -900};

    EXPECT_TRUE(linesOf(away).empty());
}

// Through the real bar and the real grid, which is where it matters.
// A press that puts the game menu away lays nothing under it.
TEST_F(PaletteSinkTest, APressThatPutsTheGameMenuAwayBuildsNothing)
{
    constexpr Cell target{.x = 4, .y = 4};

    pressOn(widgets::kGameMenu);
    clickAt(target, MouseButton::Left);

    EXPECT_FALSE(paths.has(target));
    EXPECT_TRUE(buildings().empty());

    // And the next press is an ordinary one again.
    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
}
