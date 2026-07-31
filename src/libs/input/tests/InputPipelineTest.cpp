#include "antwika/input/InputPipeline.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/Events.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputEventCodec.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/PointerHint.hpp"
#include "antwika/input/PointerHintChannel.hpp"
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
using antwika::input::PointerHint;
using antwika::input::PointerHintChannel;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::fakes::FakeInputBackend;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
namespace events = antwika::input::events;

namespace
{
    const InputEventCodec kCodec;

    // What apps/game asks for: both thinners, and Escape ends the run.
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

    // What a run put through the pipeline, as a recorder would hold it.
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
} // namespace

TEST(InputPipelineTest, EventsFor_PassesTheInnerSourceThroughUntouched)
{
    // Nothing asked for is nothing attached, not a stack of no-ops.
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
    // A replay already holds its input; reading a device too doubles it.
    ReplaySource inner({});
    FakeInputBackend backend({moved(4, 5)});

    InputPipeline pipeline(
        inner, backend, kCodec, InputPipelineOptions{.readsDevice = false});

    EXPECT_TRUE(pipeline.eventsFor(0).empty());
}

TEST(InputPipelineTest, EventsFor_ThinsWhatTheDeviceReportedNotJustTheFile)
{
    // The bug 277c54b fixed: a thinner built inside LiveInputSource.
    // It sees the scripted file alone, and so thins nothing at all.
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
    // apps/life wants this off: a drag toggles every cell it crosses.
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
    // The two branches must differ in nothing but reading a device.
    // A thinner on one branch alone shows up here as a replay adrift.
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

    // The same options the run had, less the device it no longer has.
    InputPipelineOptions replaying = kEverything;
    replaying.readsDevice = false;

    ReplaySource recordedSource(recorded);
    FakeInputBackend untouched;
    InputPipeline replayed(recordedSource, untouched, kCodec, replaying);

    EXPECT_EQ(drain(replayed, 3), recorded);
}

TEST(InputPipelineTest, EventsFor_PublishesTheMotionTheGateHoldsBack)
{
    // The whole point of the channel, in one assertion.
    // Nothing reaches the stream, so nothing reaches a recording.
    // The pointer's position still reaches whatever draws.
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
    // What "off by default" has to mean, said as an assertion.
    // Naming a channel changes the picture and not one byte of the file.
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

    EXPECT_EQ(drain(observed, 3), drain(plain, 3));

    // And the position the file does not hold is the one it has.
    EXPECT_EQ(
        channel.forRenderingOnly(),
        (PointerHint{.position = {.x = 9, .y = 9}}));
}

TEST(InputPipelineTest, EventsFor_PublishesWithNeitherThinnerAttached)
{
    // The channel is independent of the two thinning decorators.
    // An app taking no thinning at all may still draw from it.
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
