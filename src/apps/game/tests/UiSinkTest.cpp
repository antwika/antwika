#include <gtest/gtest.h>

#include <cstdint>
#include <optional>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/UiSink.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::Camera;
using antwika::game::InputFold;
using antwika::game::Toolbar;
using antwika::game::UiOverlay;
using antwika::game::UiSink;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace widgets = antwika::game::widgets;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};
    constexpr Point kHome{.x = 512, .y = 48};

    // Away from the bar, which sits in the top-left corner.
    constexpr Position kOnTheGrid{.x = 1000, .y = 600};

    class UiSinkTest : public ::testing::Test
    {
    protected:
        // Where a button is, is the layout's business.
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

                    if (toolbar.describe(kCanvas, pointer, camera)
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
        // What the sink reads is what the fold was just given.
        void dispatch(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void send(const InputEvent &event)
        {
            dispatch(TickEvent{.tick = 0, .event = codec.encode(event)});
        }

        void pressOn(WidgetId id)
        {
            const auto at = pixelOn(id);

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

        Camera camera{kHome};
        UiOverlay overlay{kCanvas};
        InputEventCodec codec;
        InputFold input{codec};
        Toolbar toolbar;
        UiSink sink{camera, overlay, input, toolbar, camera};
    };
} // namespace

TEST_F(UiSinkTest, Press_ZoomsInOnTheZoomInButton)
{
    const auto before = camera.zoomLevel();

    pressOn(widgets::kZoomIn);

    EXPECT_EQ(before + 1, camera.zoomLevel());
}

TEST_F(UiSinkTest, Press_ZoomsOutOnTheZoomOutButton)
{
    const auto before = camera.zoomLevel();

    pressOn(widgets::kZoomOut);

    EXPECT_EQ(before - 1, camera.zoomLevel());
}

TEST_F(UiSinkTest, Press_PutsTheCameraBackOnTheResetButton)
{
    const auto home = camera;

    camera.panBy(100, 100);
    camera.zoomOut();
    pressOn(widgets::kResetView);

    EXPECT_EQ(home, camera);
}

TEST_F(UiSinkTest, Press_LeavesTheCameraAloneAwayFromTheBar)
{
    const auto before = camera;

    send(PointerMoved{.position = kOnTheGrid});
    send(
        PointerButtonPressed{
            .button = MouseButton::Left, .position = kOnTheGrid});

    EXPECT_EQ(before, camera);
    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST_F(UiSinkTest, Press_ReportsTheBarCoveringWhatWasClicked)
{
    pressOn(widgets::kZoomIn);

    EXPECT_TRUE(overlay.pointerOverUi());
}

// A right-click is the grid's gesture, and never a button's.
TEST_F(UiSinkTest, RightPress_ActivatesNothing)
{
    const auto before = camera;
    const auto at = pixelOn(widgets::kZoomIn);

    send(PointerMoved{.position = at});
    send(
        PointerButtonPressed{
            .button = MouseButton::Right, .position = at});

    EXPECT_EQ(before, camera);
}

// Activation is on the press, so a release is not one.
// A release still says where the pointer is, hence no movement first.
TEST_F(UiSinkTest, Release_ActivatesNothing)
{
    const auto before = camera;

    send(
        PointerButtonReleased{
            .button = MouseButton::Left,
            .position = pixelOn(widgets::kZoomIn)});

    EXPECT_EQ(before, camera);
    EXPECT_TRUE(overlay.pointerOverUi());
}

// A press carries its own position.
// So it can be the first thing that ever says where the pointer is.
TEST_F(UiSinkTest, Press_NeedsNoMovementBeforeIt)
{
    const auto before = camera.zoomLevel();

    send(
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = pixelOn(widgets::kZoomIn)});

    EXPECT_EQ(before + 1, camera.zoomLevel());
}

TEST_F(UiSinkTest, Tick_DrawsTheBarEvenBeforeAnythingIsClicked)
{
    tick();

    EXPECT_FALSE(overlay.commands().empty());
}

// The label has to show the zoom the tick ends at.
// Otherwise the picture would trail the state by a tick.
TEST_F(UiSinkTest, Tick_DescribesTheBarAgainAfterTheStateChanged)
{
    pressOn(widgets::kZoomIn);
    const auto afterPress = overlay.commands();

    tick();

    EXPECT_EQ(afterPress, overlay.commands());
}

// A key says nothing about where the pointer is.
// So one arriving first must not put it in the corner the bar is in.
TEST_F(UiSinkTest, KeyPress_LeavesThePointerNowhere)
{
    send(KeyPressed{.key = antwika::input::Key::A});

    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST_F(UiSinkTest, Handle_IgnoresAnEventThatIsNotInput)
{
    const auto before = camera;

    dispatch(
        TickEvent{.tick = 0, .event = Event{.name = "game.started"}});

    EXPECT_EQ(before, camera);
    EXPECT_TRUE(overlay.commands().empty());
}
