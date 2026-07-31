#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/engine/Events.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/MainMenuSink.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::engine::StopSignal;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::InputFold;
using antwika::game::MainMenuScene;
using antwika::game::MainMenuSink;
using antwika::game::UiOverlay;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace menuWidgets = antwika::game::menuWidgets;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};

    // The corner: the menu's card is centred, so nothing is there.
    constexpr Position kOffEveryItem{.x = 2, .y = 2};

    class MainMenuSinkTest : public ::testing::Test
    {
    protected:
        // Where an item is, is the layout's business.
        // So a test looks for a pixel that hits the one it means.
        [[nodiscard]] Position pixelOn(WidgetId id) const
        {
            for (std::int32_t y = 0;
                 y < static_cast<std::int32_t>(kCanvas.height);
                 y += 4)
            {
                for (std::int32_t x = 0;
                     x < static_cast<std::int32_t>(kCanvas.width);
                     x += 4)
                {
                    const Pointer pointer{.position = Point{.x = x, .y = y}};

                    if (scene.describe(kCanvas, pointer)
                            .interactions.hovered
                        == id)
                    {
                        return Position{.x = x, .y = y};
                    }
                }
            }

            return Position{};
        }

        // Through the fold first, as bootstrap() registers it.
        void dispatch(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void send(const InputEvent &event)
        {
            dispatch(TickEvent{.tick = 0, .event = codec.encode(event)});
        }

        void pressAt(Position at)
        {
            send(PointerMoved{.position = at});
            send(
                PointerButtonPressed{
                    .button = MouseButton::Left, .position = at});
        }

        void tick()
        {
            dispatch(
                TickEvent{
                    .tick = 0,
                    .event =
                        Event{.name = antwika::engine::events::kTick}});
        }

        InputEventCodec codec;
        AppModeState mode;
        UiOverlay overlay{kCanvas};
        InputFold input{codec};
        MainMenuScene scene;
        StopSignal stop;
        MainMenuSink sink{mode, overlay, input, scene, stop};
    };
} // namespace

TEST_F(MainMenuSinkTest, TheMenuIsDescribedForTheRendererEveryTick)
{
    EXPECT_TRUE(overlay.commands().empty());

    tick();

    EXPECT_FALSE(overlay.commands().empty());
}

TEST_F(MainMenuSinkTest, PressingNewGameAsksForThePlayingMode)
{
    pressAt(pixelOn(menuWidgets::kNewGame));

    // Staged, not applied: the same click must not also reach the grid.
    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::CityMap);

    // And applied at the boundary, which the state itself owns.
    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});
    EXPECT_EQ(mode.mode(), AppMode::CityMap);
}

TEST_F(MainMenuSinkTest, PressingQuitStopsTheRun)
{
    EXPECT_FALSE(stop.stopped());

    pressAt(pixelOn(menuWidgets::kQuit));

    EXPECT_TRUE(stop.stopped());
}

TEST_F(MainMenuSinkTest, APressThatMissesEveryItemChangesNothing)
{
    pressAt(kOffEveryItem);

    EXPECT_EQ(mode.next(), AppMode::MainMenu);
    EXPECT_FALSE(stop.stopped());
}

TEST_F(MainMenuSinkTest, PressingWorldMapAsksForTheWorldMapMode)
{
    pressAt(pixelOn(menuWidgets::kWorldMap));

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::WorldMap);
}

TEST_F(MainMenuSinkTest, PressingLoadGameAsksForTheSaveScreen)
{
    pressAt(pixelOn(menuWidgets::kLoadGame));

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::SaveLoad);
}

// Every item now names a widget, and each names its own.
// A press walking down the card therefore hits four different ones.
TEST_F(MainMenuSinkTest, EveryItemIsReachableAndDistinct)
{
    const auto newGame = pixelOn(menuWidgets::kNewGame);
    const auto load = pixelOn(menuWidgets::kLoadGame);
    const auto world = pixelOn(menuWidgets::kWorldMap);
    const auto quit = pixelOn(menuWidgets::kQuit);

    EXPECT_LT(newGame.y, load.y);
    EXPECT_LT(load.y, world.y);
    EXPECT_LT(world.y, quit.y);
}

TEST_F(MainMenuSinkTest, NothingHappensOutsideTheMainMenuMode)
{
    mode.request(AppMode::CityMap);
    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});

    tick();
    pressAt(pixelOn(menuWidgets::kQuit));

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(stop.stopped());
}

// A non-input event carries nothing the fold could hand over.
TEST_F(MainMenuSinkTest, AnEventThatIsNotInputIsIgnored)
{
    dispatch(TickEvent{
        .tick = 0, .event = Event{.name = "game.score_increment"}});

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}

// A press is the left button's, and only the left button's.
TEST_F(MainMenuSinkTest, AMiddlePressActivatesNothing)
{
    const auto at = pixelOn(menuWidgets::kQuit);

    send(PointerMoved{.position = at});
    send(
        PointerButtonPressed{
            .button = MouseButton::Middle, .position = at});

    EXPECT_FALSE(stop.stopped());
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}
