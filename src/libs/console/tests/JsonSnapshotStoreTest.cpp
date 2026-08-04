#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/console/JsonSnapshotStore.hpp"
#include "antwika/console/SnapshotError.hpp"

using antwika::console::JsonSnapshotStore;
using antwika::console::SnapshotError;

namespace
{
    antwika::replay::MigrationChain noMigrations()
    {
        return antwika::replay::MigrationChain({}, 1);
    }

    // One application's own failure category, as every store has.
    class TestStateError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    // A category from further down, which no seam of this one's owns.
    class DeeperError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    // What a half refuses with, when it is asked to refuse at all.
    enum class Refusal
    {
        None,
        Own,
        Deeper
    };

    // A store whose whole state is one JSON object it is handed.
    class TestStore final : public JsonSnapshotStore<TestStateError>
    {
    public:
        TestStore()
            : JsonSnapshotStore(
                  {.magic = "antwika-test-state-dump", .version = 1},
                  "antwika test state dump document",
                  noMigrations)
        {
        }

        nlohmann::json state = {{"cells", "101"}};
        std::string takenFor;
        std::string appliedFor;
        Refusal refuses = Refusal::None;

    private:
        void refuse(const std::string &what) const
        {
            if (refuses == Refusal::Own)
            {
                throw TestStateError(what);
            }

            if (refuses == Refusal::Deeper)
            {
                throw DeeperError(what);
            }
        }

        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override
        {
            takenFor = path;
            refuse("nothing to take");

            return state;
        }

        void applyState(
            const std::string &path,
            const nlohmann::json &taken) override
        {
            appliedFor = path;
            refuse("a state no session could be in");

            state = taken;
        }
    };
} // namespace

TEST(JsonSnapshotStoreTest, RoundTrip_CarriesTheConsoleAndTheState)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store.json");
    const auto path = file.path().string();
    const std::vector<std::string> history{
        "> dump_state", "dumped state to " + path};

    TestStore out;
    out.state = {{"cells", "011"}, {"paused", true}};
    out.dump(path, history);

    TestStore in;
    EXPECT_EQ(in.load(path), history);
    EXPECT_EQ(in.state, out.state);
}

TEST(JsonSnapshotStoreTest, BothHalvesAreHandedTheDocumentsPath)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_path.json");
    const auto path = file.path().string();

    TestStore store;
    store.dump(path, {});
    EXPECT_EQ(store.takenFor, path);

    (void)store.load(path);
    EXPECT_EQ(store.appliedFor, path);
}

TEST(JsonSnapshotStoreTest, Dump_RewrapsWhatTakingTheStateThrew)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_untakeable.json");

    TestStore store;
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

TEST(JsonSnapshotStoreTest, Load_RewrapsWhatApplyingTheStateThrew)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_unapplicable.json");
    const auto path = file.path().string();

    TestStore out;
    out.dump(path, {});

    TestStore in;
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

// The narrowness is the point.
// A failure from further down is not this store's state being wrong.
// So it travels on as what it is.
TEST(JsonSnapshotStoreTest, ADeeperFailureEscapesAsItself)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_deeper.json");
    const auto path = file.path().string();

    TestStore out;
    out.dump(path, {});

    TestStore in;
    in.refuses = Refusal::Deeper;

    EXPECT_THROW((void)in.load(path), DeeperError);

    in.refuses = Refusal::Deeper;
    EXPECT_THROW(in.dump(path, {}), DeeperError);
}

TEST(JsonSnapshotStoreTest, Load_RefusesAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_console_json_store_absent.json");

    TestStore store;

    EXPECT_THROW(
        (void)store.load(file.path().string()), SnapshotError);
    EXPECT_TRUE(store.appliedFor.empty());
}
