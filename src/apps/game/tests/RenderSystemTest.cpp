#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <string_view>

#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/PointerHint.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/FrameMeter.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

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
using antwika::game::roadTile;
using antwika::game::UiOverlay;
using antwika::game::WorldMapScene;
using antwika::game::WorldMapState;
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

        // Through request-then-commit, as the tick path does it.
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
                .atlas = atlas,
                .paths = paths,
                .built = built,
                .camera = camera,
                .extent = kExtent,
                .overlay = overlay,
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
        const GridScene scene{};
        const MainMenuScene menuScene{};
        const WorldMapScene worldScene{};
        const SaveLoadScene saveScene{};
        UiOverlay overlay;
        UiOverlay menuOverlay{kCanvas};
        UiOverlay saveOverlay{kCanvas};
        antwika::input::PointerHintChannel hint;
        NiceMock<MockTexture> atlas;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;

        // The subject of most of these is the grid.
        // So a run is put on it rather than clicking its way there.
        AppModeState mode{AppMode::CityMap};

        // A small world, since only the mode branch is under test.
        WorldMapState cities{antwika::game::generateWorldMap(
            antwika::game::WorldMapConfig{
                .width = 6, .height = 6, .seed = 1})};
    };
} // namespace

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

    // A resize needs no handling of its own, so long as it is re-read.
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

    // The ground alone is one blit per cell.
    // A lone road adds its own, from the tile with no links.
    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height));
    EXPECT_CALL(renderer, drawTexture(Ref(atlas), roadTile(0), _, _));

    system.update(world, 0);
}

// A mode owns the whole screen.
// So in the menu no tile is blitted at all, whatever the grid holds.
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

// Likewise for the world map: rectangles, and not one tile of any grid.
TEST_F(RenderSystemTest, Update_DrawsTheWorldMapAndNoGridInThatMode)
{
    paths.insert(Cell{.x = 0, .y = 0});
    putInMode(AppMode::WorldMap);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _)).Times(0);
    EXPECT_CALL(renderer, clear(_));

    // Thirty-six tiles, and a marker for each of the four cities.
    EXPECT_CALL(renderer, drawRect(_, _)).Times(6 * 6 + 4);
    EXPECT_CALL(renderer, present());

    system.update(world, 0);
}

// And for the save screen, which is a mode of its own as well.
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

// The ghost follows the pointer through the unrecorded hint channel.
// That channel reaches a renderer and nothing else.
TEST_F(RenderSystemTest, Update_DrawsTheGhostWhereTheHintPutIt)
{
    const auto middle = antwika::game::cellCentre(
        Cell{.x = 1, .y = 1}, camera);

    hint.publish(
        antwika::input::PointerHint{
            .position = {.x = middle.x, .y = middle.y}});

    RenderSystem system(setup());

    // Four ground tiles, and the ghost over one of them.
    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height) + 1);

    system.update(world, 0);
}

// What the bar covers, it covers from the ghost too.
TEST_F(RenderSystemTest, Update_DrawsNoGhostUnderTheToolbar)
{
    const auto middle = antwika::game::cellCentre(
        Cell{.x = 1, .y = 1}, camera);

    hint.publish(
        antwika::input::PointerHint{
            .position = {.x = middle.x, .y = middle.y}});
    overlay.set({}, true);

    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawTexture(_, _, _, _))
        .Times(static_cast<int>(kExtent.width * kExtent.height));

    system.update(world, 0);
}

// The frame rate is measured against a wall clock.
// So it is counted and drawn here, and reaches no simulation.
TEST_F(RenderSystemTest, Update_CountsAndDrawsTheFrameRateWhenAskedTo)
{
    FakeClock clock{std::chrono::time_point<std::chrono::system_clock>{}};
    antwika::game::FrameMeter meter{clock};

    auto withMeter = setup();
    withMeter.fps = meter;
    RenderSystem system(withMeter);

    // One frame opens the window, and one a second later closes it.
    system.update(world, 0);
    clock.advance(std::chrono::seconds{1});
    system.update(world, 1);

    ASSERT_EQ(1U, meter.perSecond());

    // The third frame is the one that gets to draw the answer.
    EXPECT_CALL(
        renderer, drawText(_, std::string_view{"fps 1"}, _, _));

    system.update(world, 2);
}

// The corner is drawn from the first frame on, with a placeholder in it.
// So the readout does not appear a second into a session.
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

// A run that offers no clock draws no readout.
// Which is what every caller whose subject is the picture asks for.
TEST_F(RenderSystemTest, Update_DrawsNoReadoutWithoutAMeter)
{
    RenderSystem system(setup());

    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(0);

    system.update(world, 0);
}

// The readout follows the pointer through the ghost's own channel.
// Which a replay does not reproduce, and which no sink may read.
TEST_F(RenderSystemTest, Update_WritesAReadoutForWhatTheHintIsOver)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{.x = 1, .y = 1});
    world.add<antwika::game::Building>(
        entity,
        antwika::game::Building{
            .kind = antwika::game::BuildingKind::House,
            .stock = {20, 30}});
    world.commit();

    const auto middle = antwika::game::cellCentre(
        Cell{.x = 1, .y = 1}, camera);

    hint.publish(
        antwika::input::PointerHint{
            .position = {.x = middle.x, .y = middle.y}});

    RenderSystem system(setup());

    // Its name and one line per resource it depends on.
    EXPECT_CALL(renderer, drawText(_, _, _, _)).Times(3);

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
