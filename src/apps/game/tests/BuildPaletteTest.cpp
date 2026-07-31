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
#include "antwika/game/BuildingIndex.hpp"
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
using antwika::game::BuildingSprite;
using antwika::game::buildingKindIndex;
using antwika::game::buildingTile;
using antwika::game::BuildingKind;
using antwika::game::BuildTool;
using antwika::game::buildToolIndex;
using antwika::game::BuildingIndex;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::footprintBounds;
using antwika::game::footprintOf;
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
    constexpr std::array<BuildTool, kBuildToolCount - 1> kBuildings{
        BuildTool::House,
        BuildTool::FoodSource,
        BuildTool::WaterSource,
        BuildTool::FireStation,
        BuildTool::ArchitectPost};
} // namespace

TEST(BuildToolTest, EveryToolHasItsOwnIndex)
{
    EXPECT_EQ(kBuildToolCount, 6U);
    EXPECT_EQ(buildToolIndex(BuildTool::Road), 0U);
    EXPECT_EQ(buildToolIndex(BuildTool::House), 1U);
    EXPECT_EQ(buildToolIndex(BuildTool::FoodSource), 2U);
    EXPECT_EQ(buildToolIndex(BuildTool::WaterSource), 3U);
}

TEST(BuildToolTest, EveryToolButTheRoadPlacesABuilding)
{
    EXPECT_FALSE(placesBuilding(BuildTool::Road));

    for (const auto tool : kBuildings)
    {
        EXPECT_TRUE(placesBuilding(tool));
    }
}

TEST(BuildToolTest, EveryToolButTheRoadNamesABuildingKind)
{
    EXPECT_EQ(buildingKindOf(BuildTool::House), BuildingKind::House);
    EXPECT_EQ(
        buildingKindOf(BuildTool::FoodSource), BuildingKind::FoodSource);
    EXPECT_EQ(
        buildingKindOf(BuildTool::ArchitectPost),
        BuildingKind::ArchitectPost);

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

TEST(BuildPaletteWidgetTest, EveryToolIsNamedAndHasItsOwnButton)
{
    EXPECT_EQ(toolLabel(BuildTool::Road), "road");
    EXPECT_EQ(toolLabel(BuildTool::House), "house");
    EXPECT_EQ(toolLabel(BuildTool::FoodSource), "food");
    EXPECT_EQ(toolLabel(BuildTool::WaterSource), "water");
    EXPECT_EQ(toolLabel(BuildTool::FireStation), "fire");
    EXPECT_EQ(toolLabel(BuildTool::ArchitectPost), "arch");

    EXPECT_NE(widgets::toolWidget(BuildTool::Road), widgets::kZoomIn);
    EXPECT_NE(
        widgets::toolWidget(BuildTool::Road),
        widgets::toolWidget(BuildTool::WaterSource));
}

TEST(UiOverlayToolTest, TheRoadIsSelectedUntilSomethingSaysOtherwise)
{
    UiOverlay overlay;

    EXPECT_EQ(overlay.tool(), BuildTool::Road);

    overlay.select(BuildTool::FoodSource);

    EXPECT_EQ(overlay.tool(), BuildTool::FoodSource);
}

TEST(ToolbarPaletteTest, TheSelectedButtonIsDrawnDifferently)
{
    const Toolbar toolbar;
    const Camera camera;

    const auto road =
        toolbar.describe(kCanvas, Pointer{}, camera, BuildTool::Road);
    const auto tower =
        toolbar.describe(kCanvas, Pointer{}, camera, BuildTool::WaterSource);

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

        [[nodiscard]] std::vector<BuildingSprite> buildings()
        {
            world.commit();

            return snapshotOf(world, paths, camera, kExtent).buildings;
        }

        NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;

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
            cities,
            built};
    };
} // namespace

TEST_F(PaletteSinkTest, PressingAPaletteButtonSelectsThatTool)
{
    for (const auto tool : {
             BuildTool::House,
             BuildTool::FoodSource,
             BuildTool::WaterSource,
             BuildTool::Road,
         })
    {
        pressOn(widgets::toolWidget(tool));

        EXPECT_EQ(overlay.tool(), tool);
    }
}

TEST_F(PaletteSinkTest, PressingSomethingElseLeavesTheToolAlone)
{
    pressOn(widgets::toolWidget(BuildTool::FoodSource));

    // The bar is under this one, so it selects nothing.
    pressOn(widgets::kZoomIn);

    // And this one is on the grid, so it activates no widget at all.
    clickAt(Cell{.x = 4, .y = 4}, MouseButton::Left);

    EXPECT_EQ(overlay.tool(), BuildTool::FoodSource);
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

    pressOn(widgets::toolWidget(BuildTool::WaterSource));
    clickAt(target, MouseButton::Left);

    const auto placed = buildings();

    ASSERT_EQ(placed.size(), 1U);
    EXPECT_EQ(placed[0].at, target);
    EXPECT_EQ(placed[0].kind, BuildingKind::WaterSource);
    EXPECT_EQ(paths.size(), 0U);
}

TEST_F(PaletteSinkTest, TheRoadStaysWhatALeftClickPlacesByDefault)
{
    constexpr Cell target{.x = 5, .y = 5};

    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_TRUE(buildings().empty());
}

// A block holds every cell, not only the one clicked.
// Asserted through the real sink rather than through the index.
TEST_F(PaletteSinkTest, ABlockHoldsEveryCellItCovers)
{
    constexpr Cell target{.x = 2, .y = 6};

    pressOn(widgets::toolWidget(BuildTool::FoodSource));
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
    pressOn(widgets::toolWidget(BuildTool::FoodSource));
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

    pressOn(widgets::toolWidget(BuildTool::FoodSource));
    clickAt(target, MouseButton::Left);

    EXPECT_TRUE(paths.has(target));
    EXPECT_TRUE(buildings().empty());
}

TEST_F(PaletteSinkTest, ABuildingGoesNowhereOutsideTheExtent)
{
    pressOn(widgets::toolWidget(BuildTool::FoodSource));

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
        BuildTool::WaterSource,
        false,
        kNoPaths,
        kNothingBuilt);

    EXPECT_TRUE(shown.visible);
    EXPECT_EQ(shown.at, target);
    EXPECT_EQ(shown.tool, BuildTool::WaterSource);
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
    world.add<Building>(shop, Building{.kind = BuildingKind::FoodSource});

    world.commit();

    const auto snapshot = snapshotOf(world, paths, Camera(), kExtent);

    ASSERT_EQ(snapshot.buildings.size(), 1U);
    EXPECT_EQ(snapshot.buildings[0].at, (Cell{.x = 2, .y = 3}));
    EXPECT_EQ(snapshot.buildings[0].kind, BuildingKind::FoodSource);

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
            .ghost = {},
            .hover = {}};
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
        BuildingSprite{
            .at = Cell{.x = 0, .y = 0},
            .kind = BuildingKind::FoodSource});

    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 1U);
    EXPECT_EQ(
        blits[0].source, buildingTile(BuildingKind::FoodSource));
    EXPECT_EQ(
        blits[0].destination,
        footprintBounds(
            Cell{.x = 0, .y = 0},
            footprintOf(BuildingKind::FoodSource),
            snapshot.camera));
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
        .tool = BuildTool::WaterSource,
        .visible = true,
        .valid = true};

    const auto blits = blitsOf(snapshot);

    ASSERT_EQ(blits.size(), 2U);
    EXPECT_EQ(
        blits[1].source, buildingTile(BuildingKind::WaterSource));
    EXPECT_EQ(blits[1].tint, kGhostly);
}

TEST(GridSceneBuildTest, ARoadGhostShowsTheJunctionItWouldBecome)
{
    auto snapshot = emptySnapshot();
    snapshot.paths.push_back(Cell{.x = 0, .y = 1});
    snapshot.ghost = BuildGhost{
        .at = Cell{.x = 0, .y = 0},
        .tool = BuildTool::Road,
        .visible = true,
        .valid = true};

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

TEST(BuildGhostTest, GhostFor_SaysAClearBlockWouldLand)
{
    constexpr Cell target{.x = 6, .y = 3};
    const Camera camera;

    const auto shown = ghostFor(
        hintOn(target, camera),
        camera,
        kExtent,
        BuildTool::FoodSource,
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
        BuildTool::FoodSource,
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
        BuildTool::ArchitectPost,
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
