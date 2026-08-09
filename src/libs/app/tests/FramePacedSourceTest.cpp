#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/app/FramePacingError.hpp>
#include <antwika/app/fakes/FakeFramePass.hpp>
#include <antwika/app/fakes/FakeOvershootingSleeper.hpp>
#include <antwika/app/fakes/FakePacingSink.hpp>
#include <antwika/app/fakes/FakePacingTrace.hpp>
#include <antwika/app/fakes/FakeTimedPass.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/fakes/FakeClock.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/app/FramePacedSource.hpp"

namespace antwika::app
{

    namespace
    {

        using antwika::animation::Progress;
        using antwika::app::fakes::FakeFramePass;
        using antwika::app::fakes::FakeOvershootingSleeper;
        using antwika::app::fakes::FakePacingSink;
        using antwika::app::fakes::PacedFrame;
        using antwika::app::fakes::FakePacingTrace;
        using antwika::app::fakes::FakeTimedPass;
        using antwika::app::fakes::PacingStep;
        using antwika::event::Event;
        using antwika::event::TickEvent;
        using antwika::replay::ReplaySource;
        using antwika::time::fakes::FakeClock;
        using antwika::time::fakes::FakeSleeper;
        using std::chrono::milliseconds;

        constexpr milliseconds kInterval{40};

        const auto kEpoch =
            std::chrono::time_point<std::chrono::system_clock>{};

        [[nodiscard]] std::vector<TickEvent> scripted()
        {
            return {
                TickEvent{.tick = 0, .event = Event{.name = "one"}},
                TickEvent{.tick = 1, .event = Event{.name = "two"}}};
        }

    }

    TEST(FramePacedSourceTest, Ctor_RefusesATickNothingIsDrawnOn)
    {
        ReplaySource inner({});
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        EXPECT_THROW(
            FramePacedSource(
                inner,
                pass,
                sleeper,
                clock,
                {.tickInterval = kInterval, .framesPerTick = 0}),
            FramePacingError);
    }

    TEST(FramePacedSourceTest, EventsFor_HandsBackWhatTheSourceGaveIt)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 4});

        const auto events = paced.eventsFor(0);

        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(events[0].name, "one");
    }

    TEST(FramePacedSourceTest, EventsFor_NeitherWaitsNorDrawsExtraByDefault)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(inner, pass, sleeper, clock, FramePacing{});

        (void)paced.eventsFor(0);

        EXPECT_EQ(pass.count(), 0U);
        EXPECT_EQ(
            sleeper.requested(),
            std::vector<milliseconds>{milliseconds{0}});
    }

    TEST(FramePacedSourceTest, EventsFor_DrawsNoExtraFrameWhenATickIsOne)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 1});

        (void)paced.eventsFor(0);

        EXPECT_EQ(pass.count(), 0U);
        EXPECT_EQ(sleeper.requested(), std::vector<milliseconds>{kInterval});
    }

    TEST(FramePacedSourceTest, EventsFor_DrawsEveryFrameButTheTicksOwn)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
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
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 3});

        (void)paced.eventsFor(0);

        const std::vector<milliseconds> wanted{
            milliseconds{13}, milliseconds{13}, milliseconds{14}};

        EXPECT_EQ(sleeper.requested(), wanted);
        EXPECT_EQ(sleeper.total(), kInterval);
        EXPECT_EQ(clock.now() - kEpoch, kInterval);
    }

    TEST(FramePacedSourceTest, EventsFor_SpacesFramesThatFallBetweenMillis)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 80});

        (void)paced.eventsFor(0);

        const std::vector<milliseconds> opening(
            sleeper.requested().begin(), sleeper.requested().begin() + 4);

        EXPECT_EQ(
            opening,
            (std::vector<milliseconds>{
                milliseconds{0},
                milliseconds{1},
                milliseconds{0},
                milliseconds{1}}));

        EXPECT_EQ(pass.count(), 79U);
        EXPECT_EQ(clock.now() - kEpoch, kInterval);
    }

    TEST(FramePacedSourceTest, EventsFor_DrawsItsFramesOnEveryTick)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
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
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = milliseconds{0}, .framesPerTick = 2});

        const auto events = paced.eventsFor(0);

        ASSERT_EQ(events.size(), 1U);
        EXPECT_EQ(pass.count(), 1U);
        EXPECT_EQ(sleeper.total(), milliseconds{0});
    }

    TEST(FramePacedSourceTest, EventsFor_AbsorbsASleeperThatOvershoots)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeOvershootingSleeper sleeper(clock, milliseconds{1});

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 4});

        (void)paced.eventsFor(0);

        EXPECT_EQ(
            sleeper.requested(),
            (std::vector<milliseconds>{
                milliseconds{10},
                milliseconds{9},
                milliseconds{9},
                milliseconds{9}}));

        EXPECT_EQ(clock.now() - kEpoch, kInterval + milliseconds{1});
        EXPECT_EQ(pass.count(), 3U);
    }

    TEST(FramePacedSourceTest, EventsFor_PumpsTheInputBeforeEveryFrameItDraws)
    {
        ReplaySource inner(scripted());
        FakePacingTrace trace;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            trace,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 4},
            trace);

        (void)paced.eventsFor(0);

        const std::vector<PacingStep> wanted{
            PacingStep::Pumped,
            PacingStep::Drawn,
            PacingStep::Pumped,
            PacingStep::Drawn,
            PacingStep::Pumped,
            PacingStep::Drawn};

        EXPECT_EQ(trace.taken(), wanted);
    }

    TEST(FramePacedSourceTest, EventsFor_PumpsForTheTickItIsPacing)
    {
        ReplaySource inner(scripted());
        FakePacingTrace trace;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FramePacedSource paced(
            inner,
            trace,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 3},
            trace);

        (void)paced.eventsFor(1);

        EXPECT_EQ(
            trace.ticks(),
            (std::vector<antwika::time::Tick>{1, 1}));
    }

    TEST(FramePacedSourceTest, EventsFor_TellsThePacingSinkOfEveryFrameDrawn)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);
        FakePacingSink paced;

        FramePacedSource source(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 4},
            std::nullopt,
            paced);

        (void)source.eventsFor(2);

        EXPECT_EQ(
            paced.told(),
            (std::vector<PacedFrame>{
                PacedFrame::Drawn, PacedFrame::Drawn, PacedFrame::Drawn}));
        EXPECT_EQ(
            paced.about(), (std::vector<antwika::time::Tick>{2, 2, 2}));
    }

    TEST(FramePacedSourceTest, EventsFor_TellsThePacingSinkOfEveryFrameDropped)
    {
        ReplaySource inner(scripted());
        FakeClock clock(kEpoch);
        FakeOvershootingSleeper sleeper(clock, milliseconds{5});
        FakeFramePass pass;
        FakePacingSink paced;

        FramePacedSource source(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 10},
            std::nullopt,
            paced);

        (void)source.eventsFor(0);

        const std::vector<PacedFrame> wanted{
            PacedFrame::Drawn,
            PacedFrame::Dropped,
            PacedFrame::Drawn,
            PacedFrame::Dropped,
            PacedFrame::Drawn,
            PacedFrame::Dropped,
            PacedFrame::Drawn,
            PacedFrame::Dropped,
            PacedFrame::Drawn};

        EXPECT_EQ(paced.told(), wanted);
    }

    TEST(FramePacedSourceTest, EventsFor_DoesNotPumpForAFrameItDrops)
    {
        ReplaySource inner(scripted());
        FakePacingTrace trace;
        FakeClock clock(kEpoch);
        FakeOvershootingSleeper sleeper(clock, milliseconds{5});

        FramePacedSource paced(
            inner,
            trace,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 10},
            trace);

        (void)paced.eventsFor(0);

        const std::vector<PacingStep> wanted{
            PacingStep::Pumped,
            PacingStep::Drawn,
            PacingStep::Pumped,
            PacingStep::Drawn,
            PacingStep::Pumped,
            PacingStep::Drawn,
            PacingStep::Pumped,
            PacingStep::Drawn,
            PacingStep::Pumped,
            PacingStep::Drawn};

        EXPECT_EQ(trace.taken(), wanted);
    }

    TEST(FramePacedSourceTest, EventsFor_DropsAFrameItCannotDrawInTime)
    {
        ReplaySource inner(scripted());
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        FakeTimedPass pass(clock, milliseconds{6});

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 10});

        (void)paced.eventsFor(0);

        EXPECT_EQ(
            pass.drawn(),
            (std::vector<Progress>{
                Progress(1, 10),
                Progress(3, 10),
                Progress(5, 10),
                Progress(7, 10),
                Progress(9, 10)}));

        EXPECT_EQ(sleeper.total(), milliseconds{12});
    }

}
