#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <antwika/console/fakes/FakeSnapshotStore.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/console/IJsonSnapshotStore.hpp"
#include "antwika/console/SnapshotError.hpp"

using antwika::console::IJsonSnapshotStore;
using antwika::console::SnapshotError;
using antwika::console::fakes::FakeSnapshotStore;
using antwika::console::fakes::Refusal;

namespace
{
    antwika::replay::MigrationChain noMigrations()
    {
        return antwika::replay::MigrationChain({}, 1);
    }

    class TestStateError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class DeeperError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}

TEST(IJsonSnapshotStoreTest, RoundTrip_CarriesTheConsoleAndTheState)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store.json");
    const auto path = file.path().string();
    const std::vector<std::string> history{
        "> dump_state", "dumped state to " + path};

    FakeSnapshotStore<TestStateError, DeeperError> out{noMigrations};
    out.state = {{"cells", "011"}, {"paused", true}};
    out.dump(path, history);

    FakeSnapshotStore<TestStateError, DeeperError> in{noMigrations};
    EXPECT_EQ(in.load(path), history);
    EXPECT_EQ(in.state, out.state);
}

TEST(IJsonSnapshotStoreTest, Dump_HandsBothHalvesTheDocumentsPath)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_path.json");
    const auto path = file.path().string();

    FakeSnapshotStore<TestStateError, DeeperError> store{noMigrations};
    store.dump(path, {});
    EXPECT_EQ(store.takenFor, path);

    (void)store.load(path);
    EXPECT_EQ(store.appliedFor, path);
}

TEST(IJsonSnapshotStoreTest, Dump_RewrapsWhatTakingTheStateThrew)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_untakeable.json");

    FakeSnapshotStore<TestStateError, DeeperError> store{noMigrations};
    store.refuses = Refusal::Own;

    try
    {
        store.dump(file.path().string(), {});
        FAIL() << "the refused taking should have ended the dump";
    }
    catch (const SnapshotError &failed)
    {
        EXPECT_STREQ(failed.what(), "nothing to take");
    }
}

TEST(IJsonSnapshotStoreTest, Load_RewrapsWhatApplyingTheStateThrew)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_unapplicable.json");
    const auto path = file.path().string();

    FakeSnapshotStore<TestStateError, DeeperError> out{noMigrations};
    out.dump(path, {});

    FakeSnapshotStore<TestStateError, DeeperError> in{noMigrations};
    in.refuses = Refusal::Own;

    try
    {
        (void)in.load(path);
        FAIL() << "the refused state should have ended the load";
    }
    catch (const SnapshotError &failed)
    {
        EXPECT_STREQ(failed.what(), "a state no session could be in");
    }
}

TEST(IJsonSnapshotStoreTest, LoadAndDump_LetADeeperFailureEscape)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_deeper.json");
    const auto path = file.path().string();

    FakeSnapshotStore<TestStateError, DeeperError> out{noMigrations};
    out.dump(path, {});

    FakeSnapshotStore<TestStateError, DeeperError> in{noMigrations};
    in.refuses = Refusal::Deeper;

    EXPECT_THROW((void)in.load(path), DeeperError);

    in.refuses = Refusal::Deeper;
    EXPECT_THROW(in.dump(path, {}), DeeperError);
}

TEST(IJsonSnapshotStoreTest, Load_RefusesAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_absent.json");

    FakeSnapshotStore<TestStateError, DeeperError> store{noMigrations};

    EXPECT_THROW(
        (void)store.load(file.path().string()), SnapshotError);
    EXPECT_TRUE(store.appliedFor.empty());
}
