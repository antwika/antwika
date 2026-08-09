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
}

TEST(AppModeTest, Mode_StartsAtTheMainMenu)
{
    const AppModeState mode;

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::MainMenu);
}

TEST(AppModeTest, Mode_MayStartSomewhereElse)
{
    const AppModeState mode{AppMode::CityMap};

    EXPECT_EQ(mode.mode(), AppMode::CityMap);
}

TEST(AppModeTest, Request_HoldsUntilTheTickBoundary)
{
    AppModeState mode;

    mode.request(AppMode::CityMap);

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
    EXPECT_EQ(mode.next(), AppMode::CityMap);
}

TEST(AppModeTest, Handle_AppliesWhateverWasStagedOnATick)
{
    AppModeState mode;
    mode.request(AppMode::CityMap);

    mode.handle(tick());

    EXPECT_EQ(mode.mode(), AppMode::CityMap);
}

TEST(AppModeTest, Handle_LeavesTheModeOnAnyOtherEvent)
{
    AppModeState mode;
    mode.request(AppMode::CityMap);

    mode.handle(TickEvent{
        .tick = 0, .event = Event{.name = "input.pointer_down"}});

    EXPECT_EQ(mode.mode(), AppMode::MainMenu);
}

TEST(AppModeTest, Handle_ChangesNothingWithNothingStaged)
{
    AppModeState mode{AppMode::CityMap};

    mode.handle(tick());

    EXPECT_EQ(mode.mode(), AppMode::CityMap);
}
