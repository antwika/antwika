#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <optional>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/mocks/MockTickEventSink.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/InputFold.hpp"
#include "antwika/game/MainMenu.hpp"
#include "antwika/game/MenuSink.hpp"
#include "antwika/game/MenuState.hpp"
#include "antwika/game/UiOverlay.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::event::mocks::MockTickEventSink;
using antwika::game::InputFold;
using antwika::game::kMenuKey;
using antwika::game::MainMenu;
using antwika::game::MenuEntry;
using antwika::game::MenuLanguage;
using antwika::game::MenuSink;
using antwika::game::MenuState;
using antwika::game::UiOverlay;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;
namespace menuWidgets = antwika::game::menuWidgets;

namespace
{
    constexpr Size kCanvas{.width = 240, .height = 160};

    class MenuSinkTest : public ::testing::Test
    {
    protected:
        // Where an entry is, is the layout's business.
        // So a test looks for a pixel that hits the one it means.
        [[nodiscard]] Position pixelOn(WidgetId id) const
        {
            for (std::int32_t y = 0;
                 y < static_cast<std::int32_t>(kCanvas.height);
                 y += 2)
            {
                for (std::int32_t x = 0;
                     x < static_cast<std::int32_t>(kCanvas.width);
                     x += 2)
                {
                    const Pointer pointer{.position = Point{.x = x, .y = y}};

                    if (menu.describe(kCanvas, pointer, state)
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
            dispatch(TickEvent{.tick = tick, .event = codec.encode(event)});
        }

        void toggle()
        {
            send(KeyPressed{.key = kMenuKey});
        }

        void pressOn(WidgetId id)
        {
            const auto at = pixelOn(id);

            send(PointerMoved{.position = at});
            send(
                PointerButtonPressed{
                    .button = MouseButton::Left, .position = at});
        }

        void step()
        {
            dispatch(
                TickEvent{
                    .tick = tick,
                    .event =
                        Event{.name = antwika::engine::events::kTick}});
            ++tick;
        }

        antwika::time::Tick tick = 0;
        MenuState state;
        UiOverlay overlay{kCanvas};
        InputEventCodec codec;
        InputFold input{codec};
        MainMenu menu;
        MenuSink sink{state, overlay, input, menu};
    };

    // The menu in the wiring it ships in.
    // Registered in place of the toolbar's sink, and in front of it.
    class ModalMenuSinkTest : public MenuSinkTest
    {
    protected:
        ::testing::NiceMock<MockTickEventSink> toolbar;
        MenuSink modal{state, overlay, input, menu, toolbar};

        void dispatchModal(const TickEvent &event)
        {
            input.handle(event);
            modal.handle(event);
        }

        void sendModal(const InputEvent &event)
        {
            dispatchModal(
                TickEvent{.tick = tick, .event = codec.encode(event)});
        }
    };
} // namespace

TEST_F(MenuSinkTest, MenuKey_PutsTheMenuUp)
{
    toggle();

    EXPECT_TRUE(state.open);
    EXPECT_FALSE(overlay.commands().empty());
}

// A modal covers the canvas, so the grid gets nothing while it is up.
// The click that would have reached the grid is what says so.
// A press carries its own position, and this sink runs before GridSink.
TEST_F(MenuSinkTest, Press_ReportsTheMenuCoveringWhatWasClicked)
{
    toggle();

    send(
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = Position{
                .x = static_cast<std::int32_t>(kCanvas.width) - 1,
                .y = static_cast<std::int32_t>(kCanvas.height) - 1}});

    EXPECT_TRUE(overlay.pointerOverUi());
}

TEST_F(MenuSinkTest, MenuKey_TakesTheMenuDownAgain)
{
    toggle();
    toggle();

    EXPECT_FALSE(state.open);
    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(overlay.pointerOverUi());
}

// Whether or not a game is under way, which is the point of one key.
TEST_F(MenuSinkTest, MenuKey_PutsTheMenuUpDuringAGameToo)
{
    state.gameBegun = true;

    toggle();

    EXPECT_TRUE(state.open);
}

// The release is not a second press.
TEST_F(MenuSinkTest, MenuKeyRelease_LeavesTheMenuAlone)
{
    toggle();

    send(KeyReleased{.key = kMenuKey});

    EXPECT_TRUE(state.open);
}

TEST_F(MenuSinkTest, OtherKey_LeavesTheMenuAlone)
{
    send(KeyPressed{.key = Key::A});

    EXPECT_FALSE(state.open);
    EXPECT_TRUE(overlay.commands().empty());
}

TEST_F(MenuSinkTest, Tick_DrawsNothingWhileTheMenuIsDown)
{
    step();

    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST_F(MenuSinkTest, Tick_DescribesTheMenuAgainForTheRenderer)
{
    toggle();
    overlay.set({}, false);

    step();

    EXPECT_FALSE(overlay.commands().empty());
}

TEST_F(MenuSinkTest, Press_StartsAGameOnPlayGame)
{
    toggle();

    pressOn(menuWidgets::kPlayGame);

    EXPECT_EQ(MenuEntry::PlayGame, state.activated);
    EXPECT_TRUE(state.gameBegun);
    EXPECT_FALSE(state.open);
    EXPECT_TRUE(overlay.commands().empty());
    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST_F(MenuSinkTest, Press_ReportsLoadingWithoutOpeningAnything)
{
    toggle();

    pressOn(menuWidgets::kLoadReplay);

    EXPECT_EQ(MenuEntry::LoadReplay, state.activated);
    EXPECT_FALSE(state.gameBegun);
    EXPECT_FALSE(state.open);
}

// Saving leaves the session where it was, so the menu stays up.
TEST_F(MenuSinkTest, Press_KeepsTheMenuUpOnSaveReplay)
{
    toggle();

    pressOn(menuWidgets::kSaveReplay);

    EXPECT_EQ(MenuEntry::SaveReplay, state.activated);
    EXPECT_TRUE(state.open);
    EXPECT_FALSE(overlay.commands().empty());
}

TEST_F(MenuSinkTest, Press_GoesBackToTheGameOnResume)
{
    state.gameBegun = true;
    toggle();

    pressOn(menuWidgets::kResumeGame);

    EXPECT_EQ(MenuEntry::ResumeGame, state.activated);
    EXPECT_TRUE(state.gameBegun);
    EXPECT_FALSE(state.open);
}

// Playing is what makes something there to resume.
TEST_F(MenuSinkTest, Press_OffersResumeOnlyOnceAGameHasBegun)
{
    toggle();
    EXPECT_EQ(Position{}, pixelOn(menuWidgets::kResumeGame));

    pressOn(menuWidgets::kPlayGame);
    toggle();

    EXPECT_NE(Position{}, pixelOn(menuWidgets::kResumeGame));
}

TEST_F(MenuSinkTest, Press_ChangesTheLanguageWithoutLeavingTheMenu)
{
    toggle();

    pressOn(menuWidgets::kSwedish);

    EXPECT_EQ(MenuLanguage::Swedish, state.language);
    EXPECT_TRUE(state.open);
    EXPECT_EQ(std::nullopt, state.activated);
}

TEST_F(MenuSinkTest, Press_ChangesTheLanguageBackAgain)
{
    toggle();
    pressOn(menuWidgets::kSwedish);

    pressOn(menuWidgets::kEnglish);

    EXPECT_EQ(MenuLanguage::English, state.language);
}

TEST_F(MenuSinkTest, Press_ActivatesNothingWhereThereIsNoWidget)
{
    toggle();

    send(
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = Position{.x = 1, .y = 1}});

    EXPECT_EQ(std::nullopt, state.activated);
    EXPECT_TRUE(state.open);
}

// A right-click is the grid's gesture, and never an entry's.
TEST_F(MenuSinkTest, RightPress_ActivatesNothing)
{
    toggle();

    const auto at = pixelOn(menuWidgets::kPlayGame);
    send(PointerMoved{.position = at});
    send(
        PointerButtonPressed{
            .button = MouseButton::Right, .position = at});

    EXPECT_EQ(std::nullopt, state.activated);
}

TEST_F(MenuSinkTest, Press_ActivatesNothingWhileTheMenuIsDown)
{
    send(
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = pixelOn(menuWidgets::kPlayGame)});

    EXPECT_EQ(std::nullopt, state.activated);
    EXPECT_FALSE(state.gameBegun);
    EXPECT_TRUE(overlay.commands().empty());
}

// What was activated belongs to the tick it happened in.
TEST_F(MenuSinkTest, Handle_ForgetsWhatWasActivatedOnTheNextTick)
{
    toggle();
    pressOn(menuWidgets::kSaveReplay);
    step();
    ASSERT_EQ(MenuEntry::SaveReplay, state.activated);

    step();

    EXPECT_EQ(std::nullopt, state.activated);
}

TEST_F(MenuSinkTest, Handle_IgnoresAnEventThatIsNotInput)
{
    toggle();

    dispatch(
        TickEvent{.tick = tick, .event = Event{.name = "game.started"}});

    EXPECT_TRUE(state.open);
    EXPECT_EQ(std::nullopt, state.activated);
}

// A key says nothing about where the pointer is.
// So one arriving first must not put it in a corner an entry is in.
TEST_F(MenuSinkTest, MenuKey_LeavesThePointerNowhere)
{
    toggle();

    EXPECT_EQ(std::nullopt, state.activated);
    EXPECT_FALSE(overlay.pointerOverUi());
}

TEST_F(ModalMenuSinkTest, Handle_PassesEventsOnWhileTheMenuIsDown)
{
    EXPECT_CALL(toolbar, handle(::testing::_)).Times(2);

    sendModal(PointerMoved{.position = Position{.x = 4, .y = 4}});
    dispatchModal(
        TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kTick}});
}

// A bar under a menu that covers it must not still be pressable.
TEST_F(ModalMenuSinkTest, Handle_PassesNothingOnWhileTheMenuIsUp)
{
    EXPECT_CALL(toolbar, handle(::testing::_)).Times(0);

    sendModal(KeyPressed{.key = kMenuKey});
    sendModal(
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = Position{.x = 4, .y = 4}});
    dispatchModal(
        TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kTick}});
}

// Nor is the press that put the menu away one the bar should see.
TEST_F(ModalMenuSinkTest, Handle_PassesTheClosingPressOnToNobody)
{
    sendModal(KeyPressed{.key = kMenuKey});

    EXPECT_CALL(toolbar, handle(::testing::_)).Times(0);

    sendModal(KeyPressed{.key = kMenuKey});
}
