#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/console/ConsoleMount.hpp"

using antwika::console::ConsoleGatedSink;
using antwika::console::ConsoleMount;
using antwika::console::ConsoleMountSetup;
using antwika::console::ConsolePicture;
using antwika::console::IConsoleControls;
using antwika::console::InputFold;
using antwika::console::kConsoleAnimTicks;
using antwika::console::KeyboardLayout;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;

namespace
{
    constexpr antwika::gfx::Size kCanvas{
        .width = 1024, .height = 640};

    // A store that remembers what it was asked, and answers simply.
    struct RecordingStore final : antwika::console::ISnapshotStore
    {
        std::string dumpedTo;

        void dump(
            const std::string &path,
            const std::vector<std::string> &) override
        {
            dumpedTo = path;
        }

        [[nodiscard]] std::vector<std::string> load(
            const std::string &) override
        {
            return {};
        }
    };

    // The half of the seam an application with options answers off.
    struct RebindableControls final : IConsoleControls
    {
        Key toggle = Key::F1;

        [[nodiscard]] Key toggleKey() const override
        {
            return toggle;
        }

        [[nodiscard]] Key executeKey() const override
        {
            return Key::Enter;
        }

        [[nodiscard]] KeyboardLayout keyboard() const override
        {
            return antwika::console::kDefaultKeyboardLayout;
        }
    };

    // A sink that only remembers how much got through.
    struct CountingSink final : antwika::event::ITickEventSink
    {
        int seen = 0;

        void handle(const TickEvent &) override
        {
            ++seen;
        }
    };

    // One run's console, wired as every bootstrap wires one.
    struct Harness
    {
        InputEventCodec codec;
        InputFold input{codec};
        ConsolePicture overlay{kCanvas};
        RecordingStore store;
        std::string dumpPath{"scratch_dump.json"};

        // The fold first, then the console, as the run registers them.
        static void feed(
            ConsoleMount &mount,
            InputFold &input,
            const TickEvent &ticked)
        {
            input.handle(ticked);
            mount.sink().handle(ticked);
        }

        void tick(ConsoleMount &mount)
        {
            feed(
                mount,
                input,
                TickEvent{
                    .tick = 1,
                    .event =
                        Event{.name = antwika::engine::events::kTick}});
        }

        void press(ConsoleMount &mount, Key key)
        {
            feed(
                mount,
                input,
                TickEvent{
                    .tick = 1,
                    .event = codec.encode(KeyPressed{.key = key})});
        }

        void openFully(ConsoleMount &mount, Key toggle)
        {
            press(mount, toggle);

            for (std::uint32_t step = 0; step <= kConsoleAnimTicks;
                 ++step)
            {
                tick(mount);
            }
        }
    };
} // namespace

TEST(ConsoleMountTest, Mounted_IsWhetherThereIsSomewhereToDraw)
{
    Harness harness;

    ConsoleMount without(ConsoleMountSetup{
        .overlay = std::nullopt,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});
    ConsoleMount with(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    EXPECT_FALSE(without.mounted());
    EXPECT_TRUE(with.mounted());
}

TEST(ConsoleMountTest, Sink_WithoutAnOverlayWritesNobodysPicture)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = std::nullopt,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    // A run with no console registers none of this at all.
    // Driven here anyway, since what it must not touch is the point.
    harness.openFully(mount, Key::Grave);

    EXPECT_TRUE(harness.overlay.commands().empty());
}

TEST(ConsoleMountTest, Sink_OpensOnGraveAndDrawsTheCallersPicture)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);

    EXPECT_TRUE(mount.state().acceptsText());
    EXPECT_FALSE(harness.overlay.commands().empty());
}

TEST(ConsoleMountTest, Sink_AnswersOffTheControlsTheSetupCarried)
{
    Harness harness;
    RebindableControls controls;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true,
        .controls = controls});

    // The key the shipped constants toggle on is nobody's here.
    harness.press(mount, Key::Grave);
    EXPECT_FALSE(mount.state().visible());

    harness.openFully(mount, controls.toggle);
    EXPECT_TRUE(mount.state().acceptsText());
}

TEST(ConsoleMountTest, Sink_ExecutesAgainstTheStoreAndThePathGiven)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);
    mount.state().setLine("dump_state", std::string("dump_state").size());
    harness.press(mount, Key::Enter);

    EXPECT_EQ(harness.store.dumpedTo, harness.dumpPath);
    EXPECT_EQ(
        mount.state().history().back(),
        "dumped state to scratch_dump.json");
}

TEST(ConsoleMountTest, Sink_RefusesToLoadWhereALoadIsNotPermitted)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = false});

    harness.openFully(mount, Key::Grave);
    mount.state().setLine("load_state", std::string("load_state").size());
    harness.press(mount, Key::Enter);

    EXPECT_EQ(
        mount.state().history().back(),
        "load_state: not available while recording or replaying");
}

TEST(ConsoleMountTest, Gate_TakesAKeyOnlyWhileTheConsoleIsOut)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});
    CountingSink inner;
    ConsoleGatedSink gated = mount.gate(inner);

    const TickEvent typed{
        .tick = 1,
        .event = harness.codec.encode(KeyPressed{.key = Key::A})};

    harness.input.handle(typed);
    gated.handle(typed);
    EXPECT_EQ(inner.seen, 1);

    harness.openFully(mount, Key::Grave);

    harness.input.handle(typed);
    gated.handle(typed);
    EXPECT_EQ(inner.seen, 1);
}
