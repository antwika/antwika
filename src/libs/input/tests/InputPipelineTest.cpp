#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/fakes/FakeHalvingPointerMapping.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/InputPipeline.hpp"
#include "antwika/input/Events.hpp"
#include "antwika/input/IPointerMapping.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/PointerHint.hpp"
#include "antwika/input/PointerHintChannel.hpp"
#include "antwika/input/Position.hpp"
#include "antwika/input/fakes/FakeInputBackend.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::input::InputPipelineOptions;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerHint;
using antwika::input::PointerHintChannel;
using antwika::input::PointerMoved;
using antwika::input::fakes::FakeHalvingPointerMapping;
using antwika::input::fakes::FakeInputBackend;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
namespace events = antwika::input::events;

namespace
{
    const InputEventCodec kCodec;

    constexpr InputPipelineOptions kEverything{
        .readsDevice = true,
        .coalescePointerMotion = true,
        .thinIdleMotion = true,
        .stopOnKey = Key::Escape};

    [[nodiscard]] InputEvent moved(std::int32_t x, std::int32_t y)
    {
        return PointerMoved{.position = {.x = x, .y = y}};
    }

    [[nodiscard]] Event move(std::int32_t x, std::int32_t y)
    {
        return kCodec.encode(moved(x, y));
    }

    [[nodiscard]] TickEvent at(Tick tick, Event event)
    {
        return TickEvent{.tick = tick, .event = std::move(event)};
    }

    [[nodiscard]] std::vector<std::string> namesOf(
        const std::vector<Event> &events)
    {
        std::vector<std::string> names;
        for (const auto &event : events)
        {
            names.push_back(event.name);
        }
        return names;
    }

    [[nodiscard]] std::vector<TickEvent> drain(
        InputPipeline &pipeline, Tick ticks)
    {
        std::vector<TickEvent> recorded;
        for (Tick tick = 0; tick < ticks; ++tick)
        {
            for (auto &event : pipeline.eventsFor(tick))
            {
                recorded.push_back(at(tick, std::move(event)));
            }
        }
        return recorded;
    }
}

TEST(InputPipelineTest, EventsFor_PassesTheInnerSourceThroughUntouched)
{
    ReplaySource inner({at(0, move(1, 1)), at(0, move(2, 2))});
    FakeInputBackend backend;

    InputPipeline pipeline(
        inner, backend, kCodec, InputPipelineOptions{.readsDevice = false});

    EXPECT_EQ(
        pipeline.eventsFor(0),
        (std::vector<Event>{move(1, 1), move(2, 2)}));
}

TEST(InputPipelineTest, EventsFor_ReadsTheDeviceWhenItWasAskedTo)
{
    ReplaySource inner({});
    FakeInputBackend backend({moved(4, 5)});

    InputPipeline pipeline(
        inner, backend, kCodec, InputPipelineOptions{.readsDevice = true});

    EXPECT_EQ(pipeline.eventsFor(0), (std::vector<Event>{move(4, 5)}));
}

TEST(InputPipelineTest, EventsFor_LeavesTheDeviceAloneWhenReplaying)
{
    ReplaySource inner({});
    FakeInputBackend backend({moved(4, 5)});

    InputPipeline pipeline(
        inner, backend, kCodec, InputPipelineOptions{.readsDevice = false});

    EXPECT_TRUE(pipeline.eventsFor(0).empty());
}

TEST(InputPipelineTest, EventsFor_ThinsWhatTheDeviceReportedNotJustTheFile)
{
    ReplaySource inner({});
    FakeInputBackend backend(
        {moved(1, 1),
         moved(2, 2),
         moved(3, 3),
         PointerButtonPressed{
             .button = MouseButton::Left, .position = {.x = 3, .y = 3}}});

    InputPipeline pipeline(inner, backend, kCodec, kEverything);

    EXPECT_EQ(
        namesOf(pipeline.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerMove, events::kPointerDown}));
}

TEST(InputPipelineTest, EventsFor_CoalescesOnlyWhenItWasAskedTo)
{
    ReplaySource inner({});
    FakeInputBackend backend(
        {PointerButtonPressed{.button = MouseButton::Left},
         moved(1, 1),
         moved(2, 2)});

    InputPipeline pipeline(
        inner,
        backend,
        kCodec,
        InputPipelineOptions{.coalescePointerMotion = false});

    EXPECT_EQ(
        namesOf(pipeline.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerDown,
            events::kPointerMove,
            events::kPointerMove}));
}

TEST(InputPipelineTest, EventsFor_ThinsIdleMotionOnlyWhenItWasAskedTo)
{
    ReplaySource inner({at(0, move(1, 1))});
    FakeInputBackend backend;

    InputPipeline gated(
        inner,
        backend,
        kCodec,
        InputPipelineOptions{.readsDevice = false, .thinIdleMotion = true});

    EXPECT_TRUE(gated.eventsFor(0).empty());
}

TEST(InputPipelineTest, EventsFor_StopsOnTheChosenKeyOnlyWhenOneWasNamed)
{
    ReplaySource inner({});
    FakeInputBackend backend({KeyPressed{.key = Key::Escape}});

    InputPipeline stopping(inner, backend, kCodec, kEverything);

    EXPECT_EQ(
        namesOf(stopping.eventsFor(0)),
        (std::vector<std::string>{events::kKeyDown, "engine.stop"}));
}

TEST(InputPipelineTest, EventsFor_RunsAReplayThroughTheSameStackAsTheRun)
{
    const std::vector<std::vector<InputEvent>> script{
        {moved(1, 1), moved(2, 2)},
        {PointerButtonPressed{
             .button = MouseButton::Left, .position = {.x = 2, .y = 2}},
         moved(3, 3),
         moved(4, 4)},
        {PointerButtonReleased{
             .button = MouseButton::Left, .position = {.x = 4, .y = 4}},
         moved(5, 5)}};

    ReplaySource nothingScripted({});
    FakeInputBackend backend(script);
    InputPipeline live(nothingScripted, backend, kCodec, kEverything);

    const auto recorded = drain(live, 3);

    ASSERT_FALSE(recorded.empty());

    InputPipelineOptions replaying = kEverything;
    replaying.readsDevice = false;

    ReplaySource recordedSource(recorded);
    FakeInputBackend untouched;
    InputPipeline replayed(recordedSource, untouched, kCodec, replaying);

    EXPECT_EQ(drain(replayed, 3), recorded);
}

TEST(InputPipelineTest, EventsFor_PublishesTheMotionTheGateHoldsBack)
{
    ReplaySource inner({});
    FakeInputBackend backend({moved(1, 1), moved(2, 2), moved(3, 3)});
    PointerHintChannel channel;

    InputPipelineOptions drawsAHover = kEverything;
    drawsAHover.pointerHint = channel;

    InputPipeline pipeline(inner, backend, kCodec, drawsAHover);

    EXPECT_TRUE(pipeline.eventsFor(0).empty());
    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 3, .y = 3}}));
}

TEST(InputPipelineTest, EventsFor_RecordsTheSameStreamWithAChannelAndWithout)
{
    const std::vector<std::vector<InputEvent>> script{
        {moved(1, 1), moved(2, 2)},
        {PointerButtonPressed{
             .button = MouseButton::Left, .position = {.x = 2, .y = 2}},
         moved(3, 3)},
        {PointerButtonReleased{
             .button = MouseButton::Left, .position = {.x = 3, .y = 3}},
         moved(9, 9)}};

    ReplaySource plainInner({});
    FakeInputBackend plainBackend(script);
    InputPipeline plain(plainInner, plainBackend, kCodec, kEverything);

    ReplaySource hintedInner({});
    FakeInputBackend hintedBackend(script);
    PointerHintChannel channel;
    InputPipelineOptions hinted = kEverything;
    hinted.pointerHint = channel;
    InputPipeline observed(hintedInner, hintedBackend, kCodec, hinted);

    const auto withoutChannel = drain(plain, 3);

    ASSERT_FALSE(withoutChannel.empty());
    EXPECT_EQ(drain(observed, 3), withoutChannel);

    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 9, .y = 9}}));
}

namespace
{
    [[nodiscard]] std::vector<std::vector<InputEvent>> hintScript()
    {
        return {
            {moved(1, 1), moved(2, 2)},
            {PointerButtonPressed{
                .button = MouseButton::Left, .position = {.x = 5, .y = 5}}}};
    }
}

TEST(InputPipelineTest, EventsFor_HintsTheMotionOfATickThatRecordedNothing)
{
    ReplaySource nothingScripted({});
    FakeInputBackend backend(hintScript());
    PointerHintChannel liveChannel;
    InputPipelineOptions options = kEverything;
    options.pointerHint = liveChannel;
    InputPipeline live(nothingScripted, backend, kCodec, options);

    const auto firstTick = live.eventsFor(0);

    EXPECT_TRUE(firstTick.empty());
    EXPECT_EQ(
        liveChannel.forRenderingOnly(),
        (PointerHint{.position = {.x = 2, .y = 2}}));
}

TEST(InputPipelineTest, EventsFor_HintsNothingWhenItReplaysWhatWasRecorded)
{
    ReplaySource nothingScripted({});
    FakeInputBackend backend(hintScript());
    PointerHintChannel liveChannel;
    InputPipelineOptions options = kEverything;
    options.pointerHint = liveChannel;
    InputPipeline live(nothingScripted, backend, kCodec, options);

    const auto recorded = drain(live, 2);

    ASSERT_FALSE(recorded.empty());

    InputPipelineOptions replaying = options;
    replaying.readsDevice = false;
    PointerHintChannel replayedChannel;
    replaying.pointerHint = replayedChannel;

    ReplaySource recordedSource(recorded);
    FakeInputBackend untouched;
    InputPipeline replayed(recordedSource, untouched, kCodec, replaying);

    EXPECT_TRUE(replayed.eventsFor(0).empty());
    EXPECT_EQ(replayedChannel.forRenderingOnly(), std::nullopt);
}

TEST(InputPipelineTest, EventsFor_PublishesWithNeitherThinnerAttached)
{
    ReplaySource inner({});
    FakeInputBackend backend({moved(4, 5)});
    PointerHintChannel channel;

    InputPipeline pipeline(
        inner,
        backend,
        kCodec,
        InputPipelineOptions{.pointerHint = channel});

    EXPECT_EQ(pipeline.eventsFor(0), (std::vector<Event>{move(4, 5)}));
    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 4, .y = 5}}));
}

TEST(InputPipelineTest, EventsFor_MapsWhatTheDeviceReported)
{
    ReplaySource inner({});
    FakeInputBackend backend({moved(40, 20)});
    const FakeHalvingPointerMapping mapping;

    InputPipeline pipeline(
        inner,
        backend,
        kCodec,
        InputPipelineOptions{.pointerMapping = mapping});

    EXPECT_EQ(pipeline.eventsFor(0), (std::vector<Event>{move(20, 10)}));
}

TEST(InputPipelineTest, EventsFor_PublishesTheMappedPositionToTheHint)
{
    ReplaySource inner({});
    FakeInputBackend backend({moved(40, 20)});
    PointerHintChannel channel;
    const FakeHalvingPointerMapping mapping;

    InputPipeline pipeline(
        inner,
        backend,
        kCodec,
        InputPipelineOptions{
            .pointerMapping = mapping, .pointerHint = channel});

    EXPECT_EQ(pipeline.eventsFor(0), (std::vector<Event>{move(20, 10)}));
    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 20, .y = 10}}));
}

TEST(InputPipelineTest, FramePump_RefreshesTheHintWithoutWaitingForATick)
{
    const std::vector<std::vector<InputEvent>> rounds{
        {moved(1, 1)}, {moved(2, 2)}};

    ReplaySource inner({});
    FakeInputBackend backend(rounds);
    PointerHintChannel channel;

    InputPipelineOptions drawsAHover = kEverything;
    drawsAHover.pointerHint = channel;

    InputPipeline pipeline(inner, backend, kCodec, drawsAHover);

    auto pump = pipeline.framePump();

    ASSERT_TRUE(pump.has_value());

    pump->get().pump(0);

    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 1, .y = 1}}));
}

TEST(InputPipelineTest, FramePump_IsAbsentWhenTheDeviceIsNotRead)
{
    ReplaySource inner({at(0, move(1, 1))});
    FakeInputBackend backend;

    InputPipelineOptions replaying = kEverything;
    replaying.readsDevice = false;

    InputPipeline pipeline(inner, backend, kCodec, replaying);

    EXPECT_FALSE(pipeline.framePump().has_value());
}

TEST(InputPipelineTest, EventsFor_ThinsAcrossPumpsAsThoughOneTickReadThem)
{
    const std::vector<std::vector<InputEvent>> rounds{
        {PointerButtonPressed{
            .button = MouseButton::Left, .position = {.x = 1, .y = 1}}},
        {moved(2, 2)},
        {moved(3, 3), moved(4, 4)}};

    ReplaySource inner({});
    FakeInputBackend backend(rounds);

    InputPipeline pipeline(inner, backend, kCodec, kEverything);

    auto pump = pipeline.framePump();

    ASSERT_TRUE(pump.has_value());

    pump->get().pump(0);
    pump->get().pump(0);

    EXPECT_EQ(
        namesOf(pipeline.eventsFor(0)),
        (std::vector<std::string>{
            events::kPointerDown, events::kPointerMove}));
}

TEST(InputPipelineTest, EventsFor_LeavesAReplayedPositionUnmapped)
{
    ReplaySource inner({at(0, move(40, 20))});
    FakeInputBackend backend;
    const FakeHalvingPointerMapping mapping;

    InputPipeline pipeline(
        inner,
        backend,
        kCodec,
        InputPipelineOptions{
            .readsDevice = false, .pointerMapping = mapping});

    EXPECT_EQ(pipeline.eventsFor(0), (std::vector<Event>{move(40, 20)}));
}
