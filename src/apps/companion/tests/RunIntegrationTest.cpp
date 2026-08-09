#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

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

#include "PetFixtures.hpp"
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
using antwika::companion::tests::kCanvas;
using antwika::companion::tests::kBrisk;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 40;



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

    struct WatchedTicks final
    {
        std::uint64_t ticks = 0;
        std::uint32_t meals = 0;
        std::uint32_t generation = 0;
    };

    class FakeFinishedTickWatcher final
        : public antwika::event::ITickEventSink
    {
    public:
        FakeFinishedTickWatcher(
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
}

TEST(RunIntegrationTest, RunOnce_ReachesTheRecordedCompanion)
{
    const CompanionSummary lived = runOnce();

    EXPECT_EQ(lived.ticks, 21U);
    EXPECT_EQ(lived.day, 0U);
    EXPECT_EQ(lived.hunger, 8U);
    EXPECT_EQ(lived.fun, 8U);
    EXPECT_EQ(lived.happiness, 5U);
    EXPECT_EQ(lived.energy, 18U);
    EXPECT_EQ(lived.energyCeiling, 20U);
    EXPECT_EQ(lived.meals, 1U);
    EXPECT_EQ(lived.plays, 1U);
    EXPECT_EQ(lived.disturbances, 0U);
    EXPECT_EQ(lived.pesters, 1U);
    EXPECT_EQ(lived.collapses, 0U);
    EXPECT_FALSE(lived.perished);
    EXPECT_TRUE(lived.console.empty());
}

TEST(RunIntegrationTest, RunOnce_EndsOnAStopBeforeTheCap)
{
    const CompanionSummary summary = runOnce();

    EXPECT_GT(summary.ticks, 0U);
    EXPECT_LT(summary.ticks, kMaxTicks);
    EXPECT_FALSE(summary.perished);
}

TEST(RunIntegrationTest, RunOnce_ReadsAPressByWhereItLands)
{
    const CompanionSummary summary = runOnce();

    EXPECT_EQ(summary.plays, 1U);
    EXPECT_EQ(summary.meals, 1U);
    EXPECT_EQ(summary.pesters, 1U);
    EXPECT_EQ(summary.disturbances, 0U);
}

TEST(RunIntegrationTest, Run_ShowsTheExtraSinkEveryFinishedTick)
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
                return std::make_unique<FakeFinishedTickWatcher>(
                    pet, lineage, seen);
            }});

    EXPECT_EQ(seen.ticks, summary.ticks);
    EXPECT_EQ(seen.meals, summary.meals);
    EXPECT_EQ(seen.generation, summary.generation);

    EXPECT_EQ(sleeper.requested().size(), summary.ticks);
}

TEST(RunIntegrationTest, Run_GivesTheRecorderEveryDispatchedEvent)
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

TEST(RunIntegrationTest, Run_MayEndWithAPerishedCompanion)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    const InputEventCodec codec;
    FakeSleeper sleeper;

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
