#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/companion/Companion.hpp"
#include "antwika/companion/Pet.hpp"

using antwika::companion::CompanionConfig;
using antwika::companion::CompanionSummary;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeSleeper;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 40;

    // A day of six ticks and a night of four.
    // So one scripted session reaches both the meal and the tap at night.
    constexpr PetConfig kBrisk{
        .dayTicks = 6,
        .nightTicks = 4,
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .hungerMax = 8,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .feedJoy = 1,
        .disturbCost = 1,
        .happinessMax = 6,
        .happinessStart = 4};

    TickEvent pressAt(
        const InputEventCodec &codec, const antwika::time::Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = 64, .y = 64}})};
    }

    // One tap in the daylight, onto a hungry companion.
    // One onto a sleeping one, and a stop well before the cap.
    std::vector<TickEvent> script()
    {
        const InputEventCodec codec;

        return {
            pressAt(codec, 3),
            pressAt(codec, 7),
            TickEvent{
                .tick = 20,
                .event =
                    Event{.name = antwika::engine::events::kStop}}};
    }

    // What the watcher below saw, kept outside it.
    // bootstrap() owns the sink and destroys it before returning.
    struct WatchedTicks
    {
        std::uint64_t ticks = 0;
        std::uint32_t meals = 0;
    };

    // Stands in for the renderer main.cpp hands bootstrap().
    // It is built over the Pet the session owns.
    // That does not exist before bootstrap() has made one.
    // It reads exactly what RenderSink reads.
    // So what it sees is what a frame would be drawn from.
    class FinishedTickWatcher final
        : public antwika::event::ITickEventSink
    {
    public:
        FinishedTickWatcher(const Pet &pet, WatchedTicks &seen)
            : pet(pet), seen(seen)
        {
        }

        void handle(const TickEvent &event) override
        {
            if (event.event.name != antwika::engine::events::kTick)
            {
                return;
            }

            ++seen.ticks;
            seen.meals = pet.meals();
        }

    private:
        const Pet &pet;
        WatchedTicks &seen;
    };

    CompanionSummary runOnce()
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        FakeSleeper sleeper;
        ReplaySource source(script());

        return antwika::companion::bootstrap(CompanionConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .maxTicks = kMaxTicks});
    }
} // namespace

// The requirement this project exists for, for this app.
// A session is driven entirely by what a source hands it.
// Nothing about hunger, sleep or happiness is ever recorded.
// So running the same input twice has to land on the same companion.
TEST(RunIntegrationTest, TheSameInputReachesTheSameCompanionTwice)
{
    const CompanionSummary first = runOnce();
    const CompanionSummary second = runOnce();

    EXPECT_EQ(first.ticks, second.ticks);
    EXPECT_EQ(first.hunger, second.hunger);
    EXPECT_EQ(first.happiness, second.happiness);
    EXPECT_EQ(first.meals, second.meals);
    EXPECT_EQ(first.disturbances, second.disturbances);
    EXPECT_EQ(first.pesters, second.pesters);
    EXPECT_EQ(first.perished, second.perished);
}

TEST(RunIntegrationTest, AStopEventEndsTheSessionBeforeTheCap)
{
    const CompanionSummary summary = runOnce();

    EXPECT_GT(summary.ticks, 0U);
    EXPECT_LT(summary.ticks, kMaxTicks);
    EXPECT_FALSE(summary.perished);
}

// One press landed in the daylight, on a hungry companion.
// The other landed on a sleeping one.
// The difference between them is entirely when they happened.
// That is the whole game.
TEST(RunIntegrationTest, WhenATapLandsIsWhatDecidesWhatItMeans)
{
    const CompanionSummary summary = runOnce();

    EXPECT_EQ(summary.meals, 1U);
    EXPECT_EQ(summary.disturbances, 1U);

    // Neither of the two landed on a companion that wanted nothing.
    EXPECT_EQ(summary.pesters, 0U);
}

// main.cpp hangs the renderer off the session with the extraSink factory.
// A sink drawing the companion needs state bootstrap() makes itself.
// It must be registered after the sinks that change that state.
TEST(RunIntegrationTest, TheExtraSinkSeesEveryFinishedTick)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    FakeSleeper sleeper;
    ReplaySource source(script());

    WatchedTicks seen;
    const CompanionSummary summary =
        antwika::companion::bootstrap(CompanionConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .maxTicks = kMaxTicks,
            .extraSink = [&seen](const Pet &pet)
            {
                return std::make_unique<FinishedTickWatcher>(pet, seen);
            }});

    EXPECT_EQ(seen.ticks, summary.ticks);
    EXPECT_EQ(seen.meals, summary.meals);

    // Every tick was held back, whether anything was drawn or not.
    EXPECT_EQ(sleeper.requested().size(), summary.ticks);
}

// A caller persisting a `--record` file has no pre-known script.
// It passes an optional replayRecorder instead.
// bootstrap() must register it, and only the input comes back.
// The hunger, the sleep and the happiness are all regenerated.
TEST(RunIntegrationTest, TheReplayRecorderReceivesEveryDispatchedEvent)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    FakeSleeper sleeper;
    ReplaySource source(script());
    TickEventRecorder recorder;

    const CompanionSummary summary =
        antwika::companion::bootstrap(CompanionConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .maxTicks = kMaxTicks,
            .replayRecorder = recorder});

    std::vector<TickEvent> supplied;
    for (const TickEvent &event : recorder.getEvents())
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            supplied.push_back(event);
        }
    }

    EXPECT_EQ(supplied, script());
    EXPECT_EQ(
        recorder.getEvents().size(), supplied.size() + summary.ticks);
}

// A companion is mortal, and perishing is an ordinary state.
// So it has to be reachable through the loop, not only through Pet.
TEST(RunIntegrationTest, ASessionCanEndWithACompanionThatHasPerished)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    FakeSleeper sleeper;

    // Every tap here lands on a sleeping companion.
    // There are more of them than it has happiness to spend.
    PetConfig fragile = kBrisk;
    fragile.happinessStart = 2;
    fragile.disturbCost = 1;

    const std::vector<TickEvent> taps{
        pressAt(codec, 7),
        pressAt(codec, 8),
        pressAt(codec, 9),
        TickEvent{
            .tick = 11,
            .event = Event{.name = antwika::engine::events::kStop}}};
    ReplaySource source(taps);

    const CompanionSummary summary =
        antwika::companion::bootstrap(CompanionConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = fragile,
            .maxTicks = 12});

    EXPECT_TRUE(summary.perished);
    EXPECT_EQ(summary.happiness, 0U);
    EXPECT_EQ(summary.disturbances, 2U);
}
