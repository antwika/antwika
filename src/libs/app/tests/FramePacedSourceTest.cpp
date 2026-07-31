#include "antwika/app/FramePacedSource.hpp"

#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/FramePacingError.hpp>
#include <antwika/app/fakes/FakeFramePass.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

namespace antwika::app
{

    namespace
    {

        using antwika::animation::Progress;
        using antwika::app::fakes::FakeFramePass;
        using antwika::event::Event;
        using antwika::event::TickEvent;
        using antwika::replay::ReplaySource;
        using antwika::time::fakes::FakeSleeper;
        using std::chrono::milliseconds;

        constexpr milliseconds kInterval{40};

        [[nodiscard]] std::vector<TickEvent> scripted()
        {
            return {
                TickEvent{.tick = 0, .event = Event{.name = "one"}},
                TickEvent{.tick = 1, .event = Event{.name = "two"}}};
        }

    } // namespace

    TEST(FramePacedSourceTest, Constructor_RefusesATickNothingIsDrawnOn)
    {
        ReplaySource inner({});
        FakeFramePass pass;
        FakeSleeper sleeper;

        EXPECT_THROW(
            FramePacedSource(
                inner,
                pass,
                sleeper,
                {.tickInterval = kInterval, .framesPerTick = 0}),
            FramePacingError);
    }

    TEST(FramePacedSourceTest, EventsFor_HandsBackWhatTheSourceGaveIt)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeSleeper sleeper;

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            {.tickInterval = kInterval, .framesPerTick = 4});

        const auto events = paced.eventsFor(0);

        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(events[0].name, "one");
    }

    TEST(FramePacedSourceTest, EventsFor_DrawsNoExtraFrameWhenATickIsOne)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeSleeper sleeper;

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            {.tickInterval = kInterval, .framesPerTick = 1});

        (void)paced.eventsFor(0);

        // The tick draws its own frame elsewhere.
        // So one frame a tick means this draws none of its own.
        EXPECT_EQ(pass.count(), 0U);
        EXPECT_EQ(sleeper.requested(), std::vector<milliseconds>{kInterval});
    }

    TEST(FramePacedSourceTest, EventsFor_DrawsEveryFrameButTheTicksOwn)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeSleeper sleeper;

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            {.tickInterval = kInterval, .framesPerTick = 4});

        (void)paced.eventsFor(0);

        const std::vector<Progress> wanted{
            Progress(1, 4), Progress(2, 4), Progress(3, 4)};

        EXPECT_EQ(pass.drawn(), wanted);
    }

    TEST(FramePacedSourceTest, EventsFor_WaitsAWholeIntervalHoweverItIsCut)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeSleeper sleeper;

        // Forty over three does not divide.
        // The last wait absorbs the remainder.
        // Otherwise a run would drift slower every tick.
        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            {.tickInterval = kInterval, .framesPerTick = 3});

        (void)paced.eventsFor(0);

        const std::vector<milliseconds> wanted{
            milliseconds{13}, milliseconds{13}, milliseconds{14}};

        EXPECT_EQ(sleeper.requested(), wanted);
        EXPECT_EQ(sleeper.total(), kInterval);
    }

    TEST(FramePacedSourceTest, EventsFor_DrawsItsFramesOnEveryTick)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeSleeper sleeper;

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            {.tickInterval = kInterval, .framesPerTick = 3});

        (void)paced.eventsFor(0);
        (void)paced.eventsFor(1);

        EXPECT_EQ(pass.count(), 4U);
        EXPECT_EQ(sleeper.total(), kInterval * 2);
    }

    TEST(FramePacedSourceTest, EventsFor_StillDrawsWhenNoTimeIsSpent)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeSleeper sleeper;

        // An unpaced run still wants its frames; only the waiting goes.
        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            {.tickInterval = milliseconds{0}, .framesPerTick = 2});

        const auto events = paced.eventsFor(0);

        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(pass.count(), 1U);
        EXPECT_EQ(sleeper.total(), milliseconds{0});
    }

} // namespace antwika::app
