#include <gtest/gtest.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/console/ConsoleEvents.hpp"
#include "antwika/console/ConsolePicture.hpp"
#include "antwika/console/ConsoleScene.hpp"
#include "antwika/console/ConsoleSink.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/IConsoleControls.hpp"
#include "antwika/console/InputFold.hpp"
#include "antwika/console/fakes/FakeQuietCommands.hpp"
#include "antwika/console/fakes/FakeRecordingSink.hpp"
#include "antwika/console/testing/ConsoleScript.hpp"

using antwika::console::ConsoleEvents;
using antwika::console::ConsoleSink;
using antwika::console::ConsoleSinkSetup;
using antwika::console::ConsoleState;
using antwika::console::fakes::FakeQuietCommands;
using antwika::console::fakes::FakeRecordingSink;
using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::typeText;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::time::Tick;

namespace
{
    constexpr antwika::gfx::Size kCanvas{
        .width = 1024, .height = 640};

    struct Harness final
    {
        Harness(const bool queued, const bool stoppable)
            : sink(ConsoleSinkSetup{
                  .console = console,
                  .input = input,
                  .picture = picture,
                  .scene = scene,
                  .controls = controls,
                  .commands = commands,
                  .stop = stoppable
                      ? std::optional{std::ref(stopped)}
                      : std::nullopt,
                  .events = queued
                      ? std::optional{std::ref(sent)}
                      : std::nullopt})
        {
        }

        InputEventCodec codec;
        antwika::console::InputFold input{codec};
        antwika::console::ConsolePicture picture{kCanvas};
        antwika::console::ConsoleScene scene;
        ConsoleState console;
        antwika::console::FixedConsoleControls controls;
        FakeQuietCommands commands;
        FakeRecordingSink stopped;
        ConsoleEvents sent;
        std::size_t reader{sent.open()};
        ConsoleSink sink;

        void feed(const TickEvent &event)
        {
            input.handle(event);
            sink.handle(event);
        }

        void type(const std::string_view line)
        {
            feed(keyAt(codec, 1, Key::Grave));

            for (Tick tick = 1; tick <= kConsoleAnimTicks; ++tick)
            {
                feed(TickEvent{
                    .tick = tick,
                    .event = Event{
                        .name = antwika::engine::events::kTick}});
            }

            std::vector<TickEvent> typed;
            typeText(typed, codec, kOpenTick, line);
            typed.push_back(keyAt(codec, kOpenTick, Key::Enter));

            for (const auto &event : typed)
            {
                feed(event);
            }
        }

        [[nodiscard]] std::vector<std::string> said() const
        {
            std::vector<std::string> lines;

            for (const auto &line : console.history())
            {
                if (!line.starts_with("> "))
                {
                    lines.push_back(line);
                }
            }

            return lines;
        }
    };

    [[nodiscard]] bool holds(
        const std::vector<std::string> &lines, std::string_view name)
    {
        for (const auto &line : lines)
        {
            if (line == name)
            {
                return true;
            }
        }

        return false;
    }
}

TEST(ConsoleCommandListTest, CommandList_NamesTheCommandsTheAppBrings)
{
    Harness harness{true, true};
    harness.commands.named = {"dump_state", "load_state"};

    harness.type("command list");

    const auto lines = harness.said();

    EXPECT_TRUE(holds(lines, "dump_state"));
    EXPECT_TRUE(holds(lines, "load_state"));
}

TEST(ConsoleCommandListTest, CommandList_NamesTheCommandsTheConsoleOwns)
{
    Harness harness{true, true};

    harness.type("command list");

    const auto lines = harness.said();

    EXPECT_TRUE(holds(lines, "command list"));
    EXPECT_TRUE(holds(lines, "quit"));
    EXPECT_TRUE(holds(lines, "send"));
}

TEST(ConsoleCommandListTest, CommandList_LeavesOutQuitWithNothingToStop)
{
    Harness harness{true, false};

    harness.type("command list");

    EXPECT_FALSE(holds(harness.said(), "quit"));
}

TEST(ConsoleCommandListTest, CommandList_LeavesOutSendWithNoEventQueue)
{
    Harness harness{false, true};

    harness.type("command list");

    EXPECT_FALSE(holds(harness.said(), "send"));
}

TEST(ConsoleCommandListTest, CommandList_SaysTheNamesInOrder)
{
    Harness harness{true, true};
    harness.commands.named = {"load_state", "dump_state"};

    harness.type("command list");

    const auto lines = harness.said();

    EXPECT_EQ(
        lines,
        (std::vector<std::string>{
            "command list", "dump_state", "load_state", "quit", "send"}));
}

TEST(ConsoleCommandListTest, CommandList_AsksForTheWordItWants)
{
    Harness harness{true, true};

    harness.type("command");

    EXPECT_EQ(harness.said(), (std::vector<std::string>{"command: say list"}));
}

TEST(ConsoleCommandListTest, CommandList_RefusesAWordItDoesNotKnow)
{
    Harness harness{true, true};

    harness.type("command everything");

    EXPECT_EQ(harness.said(), (std::vector<std::string>{"command: say list"}));
}
