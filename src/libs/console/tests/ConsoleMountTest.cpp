#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/console/fakes/FakeConsoleControls.hpp>
#include <antwika/console/fakes/FakeCountingSink.hpp>
#include <antwika/console/fakes/FakeRecordingStore.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>

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

    struct Harness final
    {
        InputEventCodec codec;
        InputFold input{codec};
        ConsolePicture overlay{kCanvas};
        antwika::console::fakes::FakeRecordingStore store;
        std::string dumpPath{"scratch_dump.json"};

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

        void send(
            ConsoleMount &mount, const antwika::input::InputEvent &event)
        {
            feed(
                mount,
                input,
                TickEvent{.tick = 1, .event = codec.encode(event)});
        }

        void press(ConsoleMount &mount, Key key)
        {
            send(mount, KeyPressed{.key = key});
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
}

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
    antwika::console::fakes::FakeConsoleControls controls;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true,
        .controls = controls});

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

TEST(ConsoleMountTest, Sink_QuitStopsOnTheTickTheCommandWasRunOn)
{
    Harness harness;
    antwika::event::TickEventRecorder stopped;

    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true,
        .stop = stopped});

    harness.openFully(mount, Key::Grave);
    mount.state().setLine("quit", std::string("quit").size());
    harness.press(mount, Key::Enter);

    EXPECT_EQ(
        stopped.getEvents(),
        (std::vector<TickEvent>{TickEvent{
            .tick = 1,
            .event =
                Event{.name = antwika::engine::events::kStop}}}));
}

TEST(ConsoleMountTest, Sink_QuitEchoesTheCommandAndSaysItIsQuitting)
{
    Harness harness;
    antwika::event::TickEventRecorder stopped;

    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true,
        .stop = stopped});

    harness.openFully(mount, Key::Grave);
    mount.state().setLine("quit", std::string("quit").size());
    harness.press(mount, Key::Enter);

    EXPECT_EQ(
        mount.state().history(),
        (std::vector<std::string>{"> quit", "quitting"}));
}

TEST(ConsoleMountTest, Sink_QuitIsUnknownWhereThereIsNothingToStop)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);
    mount.state().setLine("quit", std::string("quit").size());
    harness.press(mount, Key::Enter);

    EXPECT_EQ(
        mount.state().history(),
        (std::vector<std::string>{"> quit", "unknown command: quit"}));
}

TEST(ConsoleMountTest, Sink_LeavesEveryOtherCommandToTheCommandsItHolds)
{
    Harness harness;
    antwika::event::TickEventRecorder stopped;

    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true,
        .stop = stopped});

    harness.openFully(mount, Key::Grave);
    mount.state().setLine("dump_state", std::string("dump_state").size());
    harness.press(mount, Key::Enter);

    ASSERT_EQ(harness.store.dumpedTo, harness.dumpPath);
    EXPECT_TRUE(stopped.getEvents().empty());
}

TEST(ConsoleMountTest, Sink_PutsWhatTheBoardMakesOfAKeyIntoTheField)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);

    harness.press(mount, Key::H);
    harness.press(mount, Key::I);

    EXPECT_EQ(mount.state().line(), "hi");
}

TEST(ConsoleMountTest, Sink_TypesNothingForAKeyThatSpellsNoCharacter)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);

    harness.press(mount, Key::H);
    harness.press(mount, Key::ArrowLeft);

    EXPECT_EQ(mount.state().line(), "h");
    EXPECT_EQ(mount.state().caret(), 0U);
}

TEST(ConsoleMountTest, Sink_TrimsTheSpacesAroundALineBeforeRunningIt)
{
    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);

    const std::string spaced = "  dump_state  ";
    mount.state().setLine(spaced, spaced.size());
    harness.press(mount, Key::Enter);

    EXPECT_EQ(
        mount.state().history(),
        (std::vector<std::string>{
            "> dump_state", "dumped state to scratch_dump.json"}));
}

TEST(ConsoleMountTest, Sink_TakesAPressFromTheLeftButtonAndNoOther)
{
    constexpr antwika::input::Position kOnTheSheet{.x = 10, .y = 10};

    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);
    harness.send(
        mount, antwika::input::PointerMoved{.position = kOnTheSheet});

    const auto settled = harness.overlay.commands();
    ASSERT_FALSE(settled.empty());

    harness.send(
        mount,
        antwika::input::PointerButtonPressed{
            .button = antwika::input::MouseButton::Right,
            .position = kOnTheSheet});

    EXPECT_EQ(harness.overlay.commands(), settled);

    harness.send(
        mount,
        antwika::input::PointerButtonPressed{
            .button = antwika::input::MouseButton::Left,
            .position = kOnTheSheet});

    EXPECT_NE(harness.overlay.commands(), settled);
}

TEST(ConsoleMountTest, Sink_CountsAnEngineTickAsNoPressAtAll)
{
    constexpr antwika::input::Position kOnTheSheet{.x = 10, .y = 10};

    Harness harness;
    ConsoleMount mount(ConsoleMountSetup{
        .overlay = harness.overlay,
        .input = harness.input,
        .store = harness.store,
        .dumpPath = harness.dumpPath,
        .loadEnabled = true});

    harness.openFully(mount, Key::Grave);
    harness.send(
        mount, antwika::input::PointerMoved{.position = kOnTheSheet});

    const auto settled = harness.overlay.commands();
    ASSERT_FALSE(settled.empty());

    harness.tick(mount);

    EXPECT_EQ(harness.overlay.commands(), settled);
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
    antwika::console::fakes::FakeCountingSink inner;
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
