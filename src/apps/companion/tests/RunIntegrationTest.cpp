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
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/companion/Companion.hpp"
#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetLayout.hpp"

using antwika::companion::CompanionWiring;
using antwika::companion::CompanionSummary;
using antwika::companion::layoutFor;
using antwika::companion::Lineage;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::Prop;
using antwika::companion::propBox;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Point;
using antwika::gfx::Size;
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

    constexpr Size kCanvas{.width = 256, .height = 256};

    // Hungry after two ticks, and nothing else moving at all.
    // So a session ends on what the scripted presses did to it.
    constexpr PetConfig kBrisk{
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .funDecayPeriodTicks = 1000,
        .fretPeriodTicks = 1000,
        .recoverPeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .drainHappyTicks = 1000,
        .drainContentTicks = 1000,
        .drainLowTicks = 1000,
        .drainMiserableTicks = 1000,
        .hungerMax = 8,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .funMax = 8,
        .funStart = 8,
        .playFun = 2,
        .playHunger = 1,
        .playEnergy = 2,
        .energyBase = 20,
        .collapsePenalty = 10,
        .happinessMax = 6,
        .happinessStart = 4};

    Point middleOf(const Prop prop)
    {
        const auto layout = layoutFor(kCanvas);
        const auto area = propBox(*layout, prop);

        return Point{
            .x = area.origin.x
                 + static_cast<std::int32_t>(area.size.width) / 2,
            .y = area.origin.y
                 + static_cast<std::int32_t>(area.size.height) / 2};
    }

    TickEvent pressAt(
        const InputEventCodec &codec,
        const antwika::time::Tick tick,
        const Point at)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(PointerButtonPressed{
                .button = MouseButton::Left,
                .position = {.x = at.x, .y = at.y}})};
    }

    // One press on the ball, and one on the bowl once it is hungry.
    // Then one on nothing in particular, and a stop before the cap.
    std::vector<TickEvent> script()
    {
        const InputEventCodec codec;

        return {
            pressAt(codec, 3, middleOf(Prop::Ball)),
            pressAt(codec, 7, middleOf(Prop::Bowl)),
            pressAt(codec, 9, Point{.x = 128, .y = 64}),
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
        std::uint32_t generation = 0;
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
        FinishedTickWatcher(
            const Pet &pet, const Lineage &lineage, WatchedTicks &seen)
            : pet(pet), lineage(lineage), seen(seen)
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
            seen.generation = lineage.generation();
        }

    private:
        const Pet &pet;
        const Lineage &lineage;
        WatchedTicks &seen;
    };

    CompanionSummary runOnce()
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        FakeSleeper sleeper;
        ReplaySource source(script());

        return antwika::companion::bootstrap(CompanionWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks});
    }
} // namespace

// The requirement this project exists for, for this app.
// A session is driven entirely by what a source hands it.
// Nothing about hunger, fun, energy or happiness is ever recorded.
// So running the same input twice has to land on the same companion.
TEST(RunIntegrationTest, TheSameInputReachesTheSameCompanionTwice)
{
    const CompanionSummary first = runOnce();
    const CompanionSummary second = runOnce();

    EXPECT_EQ(first.ticks, second.ticks);
    EXPECT_EQ(first.day, second.day);
    EXPECT_EQ(first.hunger, second.hunger);
    EXPECT_EQ(first.fun, second.fun);
    EXPECT_EQ(first.happiness, second.happiness);
    EXPECT_EQ(first.energy, second.energy);
    EXPECT_EQ(first.energyCeiling, second.energyCeiling);
    EXPECT_EQ(first.meals, second.meals);
    EXPECT_EQ(first.plays, second.plays);
    EXPECT_EQ(first.disturbances, second.disturbances);
    EXPECT_EQ(first.pesters, second.pesters);
    EXPECT_EQ(first.collapses, second.collapses);
    EXPECT_EQ(first.perished, second.perished);
    EXPECT_EQ(first.console, second.console);
}

TEST(RunIntegrationTest, AStopEventEndsTheSessionBeforeTheCap)
{
    const CompanionSummary summary = runOnce();

    EXPECT_GT(summary.ticks, 0U);
    EXPECT_LT(summary.ticks, kMaxTicks);
    EXPECT_FALSE(summary.perished);
}

// Three presses, and three different meanings.
// The only difference between them is where they landed and when.
// That is the whole game.
TEST(RunIntegrationTest, WhereAPressLandsIsWhatDecidesWhatItMeans)
{
    const CompanionSummary summary = runOnce();

    EXPECT_EQ(summary.plays, 1U);
    EXPECT_EQ(summary.meals, 1U);
    EXPECT_EQ(summary.pesters, 1U);
    EXPECT_EQ(summary.disturbances, 0U);
}

// main.cpp hangs the renderer off the session with extraSink.
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
        antwika::companion::bootstrap(CompanionWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .canvas = kCanvas,
            .maxTicks = kMaxTicks,
            .extraSink =
                [&seen](const Pet &pet, const Lineage &lineage)
            {
                return std::make_unique<FinishedTickWatcher>(
                    pet, lineage, seen);
            }});

    EXPECT_EQ(seen.ticks, summary.ticks);
    EXPECT_EQ(seen.meals, summary.meals);
    EXPECT_EQ(seen.generation, summary.generation);

    // Every tick was held back, whether anything was drawn or not.
    EXPECT_EQ(sleeper.requested().size(), summary.ticks);
}

// A caller persisting a `--record` file has no pre-known script.
// It passes an optional replayRecorder instead.
// bootstrap() must register it, and only the input comes back.
// The hunger, the fun, the energy and the happiness are regenerated.
TEST(RunIntegrationTest, TheReplayRecorderReceivesEveryDispatchedEvent)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    FakeSleeper sleeper;
    ReplaySource source(script());
    TickEventRecorder recorder;

    const CompanionSummary summary =
        antwika::companion::bootstrap(CompanionWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = kBrisk,
            .canvas = kCanvas,
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

    // One game spends everything it has.
    // The collapse that follows takes a ceiling one collapse wide.
    PetConfig fragile = kBrisk;
    fragile.energyBase = 4;
    fragile.playEnergy = 4;
    fragile.collapsePenalty = 4;

    const std::vector<TickEvent> presses{
        pressAt(codec, 3, middleOf(Prop::Ball)),
        TickEvent{
            .tick = 6,
            .event = Event{.name = antwika::engine::events::kStop}}};
    ReplaySource source(presses);

    const CompanionSummary summary =
        antwika::companion::bootstrap(CompanionWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .sleeper = sleeper,
            .pet = fragile,
            .canvas = kCanvas,
            .maxTicks = 12});

    EXPECT_TRUE(summary.perished);
    EXPECT_EQ(summary.energy, 0U);
    EXPECT_EQ(summary.energyCeiling, 0U);
    EXPECT_EQ(summary.collapses, 1U);
}
