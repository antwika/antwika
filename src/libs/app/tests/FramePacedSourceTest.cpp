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
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/fakes/FakeClock.hpp>
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
        using antwika::time::fakes::FakeClock;
        using antwika::time::fakes::FakeSleeper;
        using std::chrono::milliseconds;

        constexpr milliseconds kInterval{40};

        const auto kEpoch =
            std::chrono::time_point<std::chrono::system_clock>{};

        /**
         * @brief A sleeper that always waits a little longer than asked.
         *
         * What every real one does: a scheduler hands the thread back
         * when it next gets round to it, not at the moment the sleep was
         * for. FakeSleeper models the perfect one, and this models the
         * only kind that exists.
         */
        class OvershootingSleeper final : public antwika::time::ISleeper
        {
        public:
            OvershootingSleeper(FakeClock &clock, milliseconds by)
                : clock(clock), by(by)
            {
            }

            void sleep(milliseconds duration) override
            {
                durations.push_back(duration);
                clock.advance(duration + by);
            }

            [[nodiscard]] const std::vector<milliseconds> &requested()
                const noexcept
            {
                return durations;
            }

        private:
            FakeClock &clock;
            milliseconds by;
            std::vector<milliseconds> durations;
        };

        /**
         * @brief A frame pass that charges the clock for its drawing.
         *
         * The one thing a sleeper cannot model on its own: a machine
         * spends time drawing as well as waiting, and whether a frame's
         * moment has gone by when it comes up is exactly what decides
         * how many of them fit inside a tick.
         */
        class TimedPass final : public IFramePass
        {
        public:
            TimedPass(FakeClock &clock, milliseconds each)
                : clock(clock), each(each)
            {
            }

            void draw(Progress subTick) override
            {
                progresses.push_back(subTick);
                clock.advance(each);
            }

            [[nodiscard]] const std::vector<Progress> &drawn()
                const noexcept
            {
                return progresses;
            }

        private:
            FakeClock &clock;
            milliseconds each;
            std::vector<Progress> progresses;
        };

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

        // The tick draws its own frame elsewhere.
        // So one frame a tick means this draws none of its own.
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

        // Forty over three does not divide.
        // The last wait is against the interval rather than a slice.
        // So it absorbs the remainder and a run does not drift.
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

        // An unpaced run still wants its frames; only the waiting goes.
        // Every frame is due at once.
        // A moment that is now has not gone by.
        // So each is drawn with no wait at all.
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

    // The whole reason a clock is injected at all.
    // Sleeping a slice a frame added the sleeper's overshoot to each.
    // So a tick outlasted its interval by a millisecond per frame.
    // And the frame rate fell short of what the pacing named.
    // Measured from the top of the tick, the next wait absorbs it.
    TEST(FramePacedSourceTest, EventsFor_AbsorbsASleeperThatOvershoots)
    {
        ReplaySource inner(scripted());
        FakeFramePass pass;
        FakeClock clock(kEpoch);
        OvershootingSleeper sleeper(clock, milliseconds{1});

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 4});

        (void)paced.eventsFor(0);

        // Ten, then nine, then nine, then nine.
        // Each wait is shortened by what the one before it overran.
        EXPECT_EQ(
            sleeper.requested(),
            (std::vector<milliseconds>{
                milliseconds{10},
                milliseconds{9},
                milliseconds{9},
                milliseconds{9}}));

        // One millisecond over rather than four, and it is the last.
        // The only overshoot with nothing after it to absorb it.
        EXPECT_EQ(clock.now() - kEpoch, kInterval + milliseconds{1});
        EXPECT_EQ(pass.count(), 3U);
    }

    // A frame due in the past is dropped rather than drawn late.
    // Which is what makes a high framesPerTick free on a slow machine.
    TEST(FramePacedSourceTest, EventsFor_DropsAFrameItCannotDrawInTime)
    {
        ReplaySource inner(scripted());
        FakeClock clock(kEpoch);
        FakeSleeper sleeper(clock);

        // Ten frames over forty milliseconds is one due every four.
        // A draw costing six overruns each due time by two.
        TimedPass pass(clock, milliseconds{6});

        FramePacedSource paced(
            inner,
            pass,
            sleeper,
            clock,
            {.tickInterval = kInterval, .framesPerTick = 10});

        (void)paced.eventsFor(0);

        // Every other one is the most that fits, so that is what runs.
        // Rather than ten of them stretching the tick to sixty.
        EXPECT_EQ(
            pass.drawn(),
            (std::vector<Progress>{
                Progress(1, 10),
                Progress(3, 10),
                Progress(5, 10),
                Progress(7, 10),
                Progress(9, 10)}));

        // Asking for ten cost no more waiting than the five that fitted.
        EXPECT_EQ(sleeper.total(), milliseconds{12});
    }

} // namespace antwika::app
