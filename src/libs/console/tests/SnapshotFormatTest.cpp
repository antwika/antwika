#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/console/SnapshotError.hpp"
#include "antwika/console/SnapshotFormat.hpp"

using antwika::console::Snapshot;
using antwika::console::SnapshotError;
using antwika::console::SnapshotFormat;

namespace
{
    antwika::replay::MigrationChain noMigrations()
    {
        return antwika::replay::MigrationChain({}, 1);
    }

    [[nodiscard]] SnapshotFormat testFormat()
    {
        return SnapshotFormat(
            {.magic = "antwika-test-state-dump", .version = 1},
            "antwika test state dump document",
            noMigrations);
    }

    [[nodiscard]] Snapshot populated()
    {
        Snapshot snapshot;

        snapshot.console = {"> dump_state", "dumped state to x"};
        snapshot.state = {{"cells", "101"}, {"paused", true}};

        return snapshot;
    }
}

TEST(SnapshotFormatTest, RoundTrip_ConsoleAndStateSurvive)
{
    const auto format = testFormat();
    const auto snapshot = populated();

    const auto restored = format.fromJson(format.toJson(snapshot));

    EXPECT_EQ(
        restored.console,
        (std::vector<std::string>{"> dump_state", "dumped state to x"}));
    EXPECT_EQ(restored.state.at("cells"), "101");
    EXPECT_EQ(restored.state.at("paused"), true);
    EXPECT_EQ(restored, snapshot);
}

TEST(SnapshotFormatTest, FromJson_RefusesAnotherApplicationsMagic)
{
    const auto format = testFormat();
    auto document = format.toJson(populated());
    document["magic"] = "antwika-game-state-dump";

    EXPECT_THROW((void)format.fromJson(document), SnapshotError);
}

TEST(SnapshotFormatTest, FromJson_RefusesADocumentFromANewerBuild)
{
    const auto format = testFormat();
    auto document = format.toJson(populated());
    document[std::string(antwika::replay::kSchemaVersionKey)] = 2U;

    EXPECT_THROW((void)format.fromJson(document), SnapshotError);
}

TEST(SnapshotFormatTest, FromJson_RefusesAMissingMember)
{
    const auto format = testFormat();
    auto document = format.toJson(populated());
    document.erase("console");

    EXPECT_THROW((void)format.fromJson(document), SnapshotError);
}

TEST(SnapshotFormatTest, OperatorEquals_ComparesBothMembers)
{
    const auto base = populated();

    const auto twin = populated();
    EXPECT_EQ(base, twin);

    auto quiet = populated();
    quiet.console.clear();
    EXPECT_NE(base, quiet);

    auto other = populated();
    other.state["paused"] = false;
    EXPECT_NE(base, other);
}

TEST(SnapshotFormatTest, Write_Read_CarryASnapshotThroughAFile)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_snapshot.json");
    const auto format = testFormat();
    const auto snapshot = populated();

    format.write(snapshot, file.path().string());

    const auto restored = format.read(file.path().string());

    EXPECT_EQ(
        restored.console,
        (std::vector<std::string>{"> dump_state", "dumped state to x"}));
    EXPECT_EQ(restored.state.at("cells"), "101");
    EXPECT_EQ(restored, snapshot);
}

TEST(SnapshotFormatTest, Read_RefusesAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_snapshot_absent.json");

    EXPECT_THROW(
        (void)testFormat().read(file.path().string()), SnapshotError);
}

TEST(SnapshotFormatTest, Read_RefusesAFileThatIsNotJson)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_snapshot_torn.json");

    {
        std::ofstream out(file.path());
        out << "{ not json";
    }

    EXPECT_THROW(
        (void)testFormat().read(file.path().string()), SnapshotError);
}
