#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>

#include "antwika/game/AppMode.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::AppMode;
using antwika::game::AppModeState;

namespace
{
    [[nodiscard]] TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }
} // namespace

// The one the application never states.
// Everything built on top of this mode machinery relies on it.
TEST(AppModeTest, ARunStartsAtTheMainMenu)
{
    const AppModeState mode;

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}

TEST(AppModeTest, ARunCanBeAskedToStartSomewhereElse)
{
    const AppModeState mode{AppMode::Playing};

    EXPECT_EQ(mode.mode(), AppMode::Playing);
}

// The whole point of staging.
// The click that leaves the menu must not be read by the mode it reveals.
TEST(AppModeTest, ARequestedModeIsNotTheModeUntilTheTickBoundary)
{
    AppModeState mode;

    mode.request(AppMode::Playing);

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::Playing);
}

TEST(AppModeTest, EngineTickAppliesWhateverWasStaged)
{
    AppModeState mode;
    mode.request(AppMode::Playing);

    mode.handle(tick());

    EXPECT_EQ(mode.mode(), AppMode::Playing);
}

TEST(AppModeTest, AnythingButATickLeavesTheModeAlone)
{
    AppModeState mode;
    mode.request(AppMode::Playing);

    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = "input.pointer_down"}});

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
}

TEST(AppModeTest, ATickWithNothingStagedChangesNothing)
{
    AppModeState mode{AppMode::Playing};

    mode.handle(tick());

    EXPECT_EQ(mode.mode(), AppMode::Playing);
}
