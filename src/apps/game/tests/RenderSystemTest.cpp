#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <string_view>

#include <antwika/animation/Progress.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/PointerHint.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/time/fakes/FakeClock.hpp>
#include <antwika/console/ConsolePicture.hpp>

#include "Translators.hpp"
#include "AtlasSpecsFixture.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/FrameMeter.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::game::testing::kTestSpecs;
using antwika::game::tests::kTranslator;

using antwika::ecs::World;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::MainMenuScene;
using antwika::game::PathIndex;
using antwika::game::RenderSetup;
using antwika::game::RenderSystem;
using antwika::game::SaveLoadScene;
using antwika::animation::Progress;
using antwika::game::Direction;
using antwika::game::roadTile;
using antwika::game::walkerTile;
using antwika::game::UiOverlay;
using antwika::game::WorldMapScene;
using antwika::game::WorldMapState;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx::mocks::MockWindow;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeClock;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

namespace
{
    constexpr Size kCanvas{.width = 320, .height = 240};
    constexpr GridExtent kExtent{.width = 2, .height = 2};

    class RenderSystemTest : public ::testing::Test
    {
    protected:
        RenderSystemTest()
        {
            ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
            ON_CALL(window, size()).WillByDefault(Return(kCanvas));
        }

        void putInMode(AppMode wanted)
        {
            mode.request(wanted);
            mode.handle(
                antwika::event::TickEvent{
                    .tick = 0,
                    .event = antwika::event::Event{
                        .name = antwika::engine::events::kTick}});
        }

        [[nodiscard]] RenderSetup setup()
        {
            return RenderSetup{
                .window = window,
                .mode = mode,
                .canvas = kCanvas,
                .scene = scene,
                .atlases =
                    {.oneByOne = atlas,
                     .twoByTwo = atlas,
                     .threeByThree = atlas,
                     .walker = atlas,
                     .specs = kTestSpecs},
                .paths = paths,
                .built = built,
                .camera = camera,
                .extent = kExtent,
                .pause = pause,
                .overlay = overlay,
                .drag = drag,
                .hint = hint,
                .menuScene = menuScene,
                .menuOverlay = menuOverlay,
                .saveScene = saveScene,
                .saveOverlay = saveOverlay,
                .worldScene = worldScene,
                .cities = cities};
        }

        NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        antwika::game::BuildingIndex built;
        Camera camera;
        const GridScene scene{kTranslator};
        const MainMenuScene menuScene{kTranslator};
        const WorldMapScene worldScene{};
        const SaveLoadScene saveScene{kTranslator};
        UiOverlay overlay;
        UiOverlay menuOverlay{kCanvas};
        UiOverlay saveOverlay{kCanvas};
        antwika::input::PointerHintChannel hint;
        NiceMock<MockTexture> atlas;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;

        AppModeState mode{AppMode::CityMap};

        antwika::game::PauseState pause;

        antwika::game::RoadDrag drag;

        WorldMapState cities{antwika::game::generateWorldMap(
            antwika::game::WorldMapConfig{
                .width = 6, .height = 6, .seed = 1})};
    };
}

TEST_F(RenderSystemTest, Update_DrawsAndThenPresentsExactlyOneFrame)
{
    RenderSystem system(setup());

    ::testing::InSequence order;
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_ReadsTheWindowsSizeEveryTick)
{
    RenderSystem system(setup());

    EXPECT_CALL(window, size())
        .WillOnce(Return(kCanvas))
        .WillOnce(Return(Size{.width = 640, .height = 480}));

    system.update(world, 0);
    system.update(world, 1);
}

TEST_F(RenderSystemTest, Update_DrawsThePathsItIsGiven)
{
    paths.insert(Cell{.x = 0, .y = 0});

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height));
    EXPECT_CALL(renderer, drawTexture(Ref(atlas), roadTile(
        kTestSpecs,
        0), _, _));

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_DrawsTheMenuAndNoGridInTheMainMenuMode)
{
    paths.insert(Cell{.x = 0, .y = 0});
    putInMode(AppMode::MainMenu);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_DrawsTheWorldMapAndNoGridInThatMode)
{
    paths.insert(Cell{.x = 0, .y = 0});
    putInMode(AppMode::WorldMap);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);
    EXPECT_CALL(renderer, clear(_));

    EXPECT_CALL(renderer, drawRect(_, _)).Times(6 * 6 + 4);
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_DrawsTheSaveScreenAndNoGridInThatMode)
{
    paths.insert(Cell{.x = 0, .y = 0});
    putInMode(AppMode::SaveLoad);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);
    EXPECT_CALL(renderer, clear(_));
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_DrawsTheGhostWhereTheHintPutIt)
{
    const auto middle = antwika::game::cellCentre(
        Cell{.x = 1, .y = 1}, camera);

    hint.publish(
        antwika::input::PointerHint{
            .position = {.x = middle.x, .y = middle.y}});

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height) + 1);

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_DrawsNoGhostUnderTheToolbar)
{
    const auto middle = antwika::game::cellCentre(
        Cell{.x = 1, .y = 1}, camera);

    hint.publish(
        antwika::input::PointerHint{
            .position = {.x = middle.x, .y = middle.y}});

    overlay.set(
        {antwika::ui::FillRect{
            .rect = Rect{
                .origin = antwika::gfx::Point{.x = 0, .y = 0},
                .size = kCanvas}}},
        {},
        false);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height));

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_DrawsAGhostTheStalePressFlagWouldHide)
{
    const auto middle = antwika::game::cellCentre(
        Cell{.x = 1, .y = 1}, camera);

    hint.publish(
        antwika::input::PointerHint{
            .position = {.x = middle.x, .y = middle.y}});

    overlay.set(
        {antwika::ui::FillRect{
            .rect = Rect{
                .origin = antwika::gfx::Point{.x = 300, .y = 0},
                .size = Size{.width = 20, .height = 240}}}},
        {},
        true);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height) + 1);
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(AnyNumber());

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_PaintsTheViewTheSetupNames)
{
    antwika::game::MapViewState mapView;
    mapView.set(antwika::game::MapView::Desirability);

    antwika::game::DesirabilityField field;
    field[Cell{.x = 1, .y = 1}] = 6;

    auto withView = setup();
    withView.view = mapView;
    withView.desirability = field;
    RenderSystem system(withView);

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(
            static_cast<int>(kExtent.width * kExtent.height) * 2 + 1);

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_PaintsNoDesirabilityWithNoFieldToRead)
{
    antwika::game::MapViewState mapView;
    mapView.set(antwika::game::MapView::Desirability);

    auto withView = setup();
    withView.view = mapView;
    RenderSystem system(withView);

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height) * 2);

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_CountsAndDrawsTheFrameRateWhenAskedTo)
{
    FakeClock clock{std::chrono::time_point<std::chrono::system_clock>{}};
    antwika::game::FrameMeter meter{clock};

    auto withMeter = setup();
    withMeter.fps = meter;
    RenderSystem system(withMeter);

    system.update(world, 0);
    clock.advance(std::chrono::seconds{1});
    system.update(world, 1);

    ASSERT_EQ(1U, meter.perSecond());

    EXPECT_CALL(
        renderer, drawText(_, std::string_view{"fps 1"}, _, _));

    system.update(world, 2);
}

TEST_F(RenderSystemTest, Update_DrawsAPlaceholderBeforeTheFirstSecond)
{
    FakeClock clock{std::chrono::time_point<std::chrono::system_clock>{}};
    antwika::game::FrameMeter meter{clock};

    auto withMeter = setup();
    withMeter.fps = meter;
    RenderSystem system(withMeter);

    EXPECT_CALL(
        renderer, drawText(_, std::string_view{"fps --"}, _, _));

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_DrawsNoReadoutWithoutAMeter)
{
    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(0);

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_WritesAReadoutForWhatTheHintIsOver)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Building>(
        entity,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House,
            .stock = {20, 30, 40}});
    world.commit();

    const auto middle = antwika::game::cellCentre(
        Cell{.x = 1, .y = 1}, camera);

    hint.publish(
        antwika::input::PointerHint{
            .position = {.x = middle.x, .y = middle.y}});

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawText(_, _, _, _))
        .Times(11 + static_cast<int>(antwika::game::kResourceCount));

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_WritesNoReadoutWithNoHintAtAll)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Building>(
        entity,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House});
    world.commit();

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(0);

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Draw_HoldsAWalkerStillWhileTheRunIsPaused)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Walker>(
        entity,
        antwika::game::Walker{
            .facing = Direction::East,
            .ticksUntilStep = antwika::game::kTicksPerStep / 2 - 1,
            .from = Cell{.x = 0, .y = 1}});
    world.commit();

    pause.set(true);

    RenderSystem system(setup());

    Rect atTick{};
    Rect between{};

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawTexture(Ref(atlas), walkerTile(
            kTestSpecs,
            Direction::East, 2), _, _))
        .WillOnce(SaveArg<2>(&atTick))
        .WillOnce(SaveArg<2>(&between));

    system.update(world, 0);
    system.draw(Progress(1, 2));

    EXPECT_EQ(between, atTick);
}

TEST_F(RenderSystemTest, Draw_SlidesAWalkerWhileTheRunIsNotPaused)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Walker>(
        entity,
        antwika::game::Walker{
            .facing = Direction::East,
            .ticksUntilStep = antwika::game::kTicksPerStep / 2 - 1,
            .from = Cell{.x = 0, .y = 1}});
    world.commit();

    RenderSystem system(setup());

    Rect atTick{};
    Rect between{};

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(
        renderer,
        drawTexture(Ref(atlas), walkerTile(
            kTestSpecs,
            Direction::East, 2), _, _))
        .WillOnce(SaveArg<2>(&atTick))
        .WillOnce(SaveArg<2>(&between));

    system.update(world, 0);
    system.draw(Progress(1, 2));

    EXPECT_NE(between, atTick);
}

TEST_F(RenderSystemTest, Update_PreviewsTheRunOfRoadBeingDraggedOut)
{
    drag.begin(Cell{.x = 0, .y = 0});
    drag.dragTo(Cell{.x = 1, .y = 1});

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height) + 3);

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_PreviewsNothingWithNoDragUnderWay)
{
    drag.begin(Cell{.x = 0, .y = 0});
    drag.finish();

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height));

    system.update(world, 0);
}

TEST_F(RenderSystemTest, Update_PaintsTheConsoleOverlayWhenOneIsOffered)
{
    antwika::console::ConsolePicture consoleOverlay{kCanvas};
    consoleOverlay.set(
        {antwika::ui::FillRect{
            .rect = Rect{
                .origin = antwika::gfx::Point{.x = 0, .y = 0},
                .size = Size{.width = kCanvas.width, .height = 120}}}});

    auto offered = setup();
    offered.consoleOverlay = consoleOverlay;
    RenderSystem system(offered);

    EXPECT_CALL(renderer, drawRect(_, _)).Times(::testing::AtLeast(1));

    system.update(world, 0);
}
