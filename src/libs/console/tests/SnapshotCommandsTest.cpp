#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

#include <antwika/console/fakes/FakeReplayingStore.hpp>

#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/ISnapshotStore.hpp"
#include "antwika/console/SnapshotCommands.hpp"
#include "antwika/console/SnapshotError.hpp"

using antwika::console::ConsoleState;
using antwika::console::consoleLoadPermitted;
using antwika::console::SnapshotCommands;
using antwika::console::SnapshotError;

namespace
{
}

TEST(SnapshotCommandsTest, ConsoleLoadPermitted_OnlyInAPlainLiveRun)
{
    EXPECT_TRUE(consoleLoadPermitted(false, false));
    EXPECT_FALSE(consoleLoadPermitted(true, false));
    EXPECT_FALSE(consoleLoadPermitted(false, true));
    EXPECT_FALSE(consoleLoadPermitted(true, true));
}

TEST(SnapshotCommandsTest, DumpState_AnswersFirstAndCarriesTheAnswer)
{
    antwika::console::fakes::FakeReplayingStore store;
    SnapshotCommands commands(store, "dump_state.json", true);
    ConsoleState console;
    console.pushHistory("> dump_state");

    commands.execute("dump_state", console);

    EXPECT_EQ(store.dumpedTo, "dump_state.json");
    EXPECT_EQ(
        store.dumpedConsole,
        (std::vector<std::string>{
            "> dump_state", "dumped state to dump_state.json"}));
    EXPECT_EQ(console.history(), store.dumpedConsole);
}

TEST(SnapshotCommandsTest, LoadState_ComesBackToTheDumpedExchange)
{
    antwika::console::fakes::FakeReplayingStore store;
    SnapshotCommands commands(store, "dump_state.json", true);
    ConsoleState console;
    console.pushHistory("> load_state");

    commands.execute("load_state", console);

    EXPECT_EQ(
        console.history(),
        (std::vector<std::string>{
            "> dump_state",
            "dumped",
            "loaded state from dump_state.json"}));
}

TEST(SnapshotCommandsTest, LoadState_IsRefusedWhileRecordingOrReplaying)
{
    antwika::console::fakes::FakeReplayingStore store;
    SnapshotCommands commands(store, "dump_state.json", false);
    ConsoleState console;
    console.pushHistory("> load_state");

    commands.execute("load_state", console);

    EXPECT_EQ(
        console.history(),
        (std::vector<std::string>{
            "> load_state",
            "load_state: not available while recording or "
            "replaying"}));
}

TEST(SnapshotCommandsTest, LoadState_AnswersAStoreThatRefuses)
{
    antwika::console::fakes::FakeReplayingStore store;
    store.refuse = true;
    SnapshotCommands commands(store, "dump_state.json", true);
    ConsoleState console;
    console.pushHistory("> load_state");

    commands.execute("load_state", console);

    EXPECT_EQ(
        console.history(),
        (std::vector<std::string>{
            "> load_state", "could not load: no such dump"}));
}

TEST(SnapshotCommandsTest, Execute_ReportsAnUnknownCommand)
{
    antwika::console::fakes::FakeReplayingStore store;
    SnapshotCommands commands(store, "dump_state.json", true);
    ConsoleState console;
    console.pushHistory("> help");

    commands.execute("help", console);

    EXPECT_EQ(
        console.history(),
        (std::vector<std::string>{"> help", "unknown command: help"}));
}

TEST(SnapshotCommandsTest, Names_NamesBothStateCommands)
{
    antwika::console::fakes::FakeReplayingStore store;
    SnapshotCommands commands(store, "dump_state.json", true);

    EXPECT_EQ(
        commands.names(),
        (std::vector<std::string>{"dump_state", "load_state"}));
}

TEST(SnapshotCommandsTest, ConsoleLoadPermitted_ReadsAPlainRunFromItsOptions)
{
    EXPECT_TRUE(consoleLoadPermitted(antwika::replay::ReplayCliOptions{}));
}

TEST(SnapshotCommandsTest, ConsoleLoadPermitted_RefusesARecordingsOptions)
{
    EXPECT_FALSE(consoleLoadPermitted(
        antwika::replay::ReplayCliOptions{
            .recordPath = "run.jsonl", .replayPath = std::nullopt}));
}

TEST(SnapshotCommandsTest, ConsoleLoadPermitted_RefusesAReplaysOptions)
{
    EXPECT_FALSE(consoleLoadPermitted(
        antwika::replay::ReplayCliOptions{
            .recordPath = std::nullopt, .replayPath = "run.jsonl"}));
}
