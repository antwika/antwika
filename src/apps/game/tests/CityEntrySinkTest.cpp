#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/CityEntrySink.hpp"
#include "antwika/game/PauseState.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::CityEntrySink;
using antwika::game::PauseState;

namespace
{
    [[nodiscard]] TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    [[nodiscard]] TickEvent somethingElse()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "input.pointer_moved"}};
    }

    // Commits a staged mode the way the tick path does.
    // Then lets the sink see the very same tick.
    // Which is the order Game.cpp registers the two in.
    void step(AppModeState &mode, CityEntrySink &sink)
    {
        mode.handle(tick());
        sink.handle(tick());
    }
} // namespace

// The whole rule, in one test: a city comes up held.
TEST(CityEntrySinkTest, ACityReachedFromAnotherScreenComesUpPaused)
{
    AppModeState mode;
    PauseState pause;
    CityEntrySink sink(mode, pause);

    mode.request(AppMode::CityMap);
    step(mode, sink);

    EXPECT_TRUE(pause.paused());
}

// The first city of a session is no exception.
// New Game is a transition like any other.
// So it needs no rule of its own.
TEST(CityEntrySinkTest, TheWorldMapIsNotTheOnlyWayIn)
{
    AppModeState mode;
    PauseState pause;
    CityEntrySink sink(mode, pause);

    mode.request(AppMode::WorldMap);
    step(mode, sink);
    ASSERT_FALSE(pause.paused());

    mode.request(AppMode::CityMap);
    step(mode, sink);

    EXPECT_TRUE(pause.paused());
}

// Held rather than toggled.
// A city entered from a paused one must not come up running.
TEST(CityEntrySinkTest, EnteringAPausedCityLeavesItPaused)
{
    AppModeState mode{AppMode::CityMap};
    PauseState pause;
    CityEntrySink sink(mode, pause);

    pause.toggle();
    mode.request(AppMode::WorldMap);
    step(mode, sink);

    mode.request(AppMode::CityMap);
    step(mode, sink);

    EXPECT_TRUE(pause.paused());
}

// Otherwise the pause button would do nothing at all.
// It is the transition that pauses, not the mode.
TEST(CityEntrySinkTest, ACityAlreadyUpIsLeftAlone)
{
    AppModeState mode{AppMode::CityMap};
    PauseState pause;
    CityEntrySink sink(mode, pause);

    step(mode, sink);
    ASSERT_FALSE(pause.paused());

    pause.toggle();
    pause.toggle();
    step(mode, sink);

    EXPECT_FALSE(pause.paused());
}

// A screen that is not a city's is not an entry.
TEST(CityEntrySinkTest, AnyOtherScreenChangesNothing)
{
    AppModeState mode;
    PauseState pause;
    CityEntrySink sink(mode, pause);

    mode.request(AppMode::SaveLoad);
    step(mode, sink);

    EXPECT_FALSE(pause.paused());
}

// The mode changes at the tick boundary, so this reads it there.
// An input event arrives before that boundary and says nothing yet.
TEST(CityEntrySinkTest, NothingButTheTickBoundaryIsRead)
{
    AppModeState mode;
    PauseState pause;
    CityEntrySink sink(mode, pause);

    mode.request(AppMode::CityMap);
    sink.handle(somethingElse());

    EXPECT_FALSE(pause.paused());
}
