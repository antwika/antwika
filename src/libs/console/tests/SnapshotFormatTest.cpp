#include <gtest/gtest.h>

#include <fstream>
#include <string>

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
} // namespace

TEST(SnapshotFormatTest, RoundTrip_ConsoleAndStateSurvive)
{
    const auto format = testFormat();
    const auto snapshot = populated();

    EXPECT_EQ(format.fromJson(format.toJson(snapshot)), snapshot);
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

TEST(SnapshotFormatTest, EqualityComparesBothMembers)
{
    const auto base = populated();

    auto quiet = populated();
    quiet.console.clear();
    EXPECT_NE(base, quiet);

    auto other = populated();
    other.state["paused"] = false;
    EXPECT_NE(base, other);
}

TEST(SnapshotFormatTest, File_RoundTripsAndRefusesWhatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_snapshot.json");
    const auto format = testFormat();
    const auto snapshot = populated();

    format.write(snapshot, file.path().string());

    EXPECT_EQ(format.read(file.path().string()), snapshot);
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
