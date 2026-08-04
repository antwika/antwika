#include <gtest/gtest.h>

#include <string>
#include <vector>

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
    // A store that remembers what it was asked, and answers simply.
    struct RecordingStore final : antwika::console::ISnapshotStore
    {
        std::string dumpedTo;
        std::vector<std::string> dumpedConsole;
        std::vector<std::string> answers{"> dump_state", "dumped"};
        bool refuse = false;

        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override
        {
            dumpedTo = path;
            dumpedConsole = console;
        }

        [[nodiscard]] std::vector<std::string> load(
            const std::string &) override
        {
            if (refuse)
            {
                throw SnapshotError("no such dump");
            }

            return answers;
        }
    };
} // namespace

TEST(SnapshotCommandsTest, ConsoleLoadPermitted_OnlyInAPlainLiveRun)
{
    EXPECT_TRUE(consoleLoadPermitted(false, false));
    EXPECT_FALSE(consoleLoadPermitted(true, false));
    EXPECT_FALSE(consoleLoadPermitted(false, true));
    EXPECT_FALSE(consoleLoadPermitted(true, true));
}

TEST(SnapshotCommandsTest, DumpState_AnswersFirstAndCarriesTheAnswer)
{
    RecordingStore store;
    SnapshotCommands commands(store, "dump_state.json", true);
    ConsoleState console;
    console.pushHistory("> dump_state");

    commands.execute("dump_state", console);

    // The answer precedes the write, so the dump carries it.
    EXPECT_EQ(store.dumpedTo, "dump_state.json");
    EXPECT_EQ(
        store.dumpedConsole,
        (std::vector<std::string>{
            "> dump_state", "dumped state to dump_state.json"}));
    EXPECT_EQ(console.history(), store.dumpedConsole);
}

TEST(SnapshotCommandsTest, LoadState_ComesBackToTheDumpedExchange)
{
    RecordingStore store;
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
    RecordingStore store;
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
    RecordingStore store;
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

TEST(SnapshotCommandsTest, AnythingElseIsUnknown)
{
    RecordingStore store;
    SnapshotCommands commands(store, "dump_state.json", true);
    ConsoleState console;
    console.pushHistory("> help");

    commands.execute("help", console);

    EXPECT_EQ(
        console.history(),
        (std::vector<std::string>{"> help", "unknown command: help"}));
}
