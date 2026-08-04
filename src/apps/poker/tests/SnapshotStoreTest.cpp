#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/Deck.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>
#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/PolicyAgent.hpp"
#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/SnapshotStore.hpp"
#include "antwika/poker/TablePrinter.hpp"

using antwika::console::SnapshotError;
using antwika::holdem::Blinds;
using antwika::holdem::Deck;
using antwika::holdem::IAgent;
using antwika::holdem::Table;
using antwika::holdem::TableRunner;
using antwika::poker::BankrollLedger;
using antwika::poker::CashGame;
using antwika::poker::PokerSnapshotStore;
using antwika::poker::PolicyAgent;
using antwika::poker::RoomConfig;
using antwika::poker::TablePrinter;
using antwika::rng::SplitMix64Rng;
using antwika::time::fakes::FakeClock;

namespace
{
    constexpr Blinds kBlinds{.small = 5, .big = 10};

    // The pieces one room is made of, exactly as bootstrap makes them.
    struct Room
    {
        explicit Room(std::uint64_t seed) : bits(seed), deck(bits)
        {
            const RoomConfig config{};

            for (std::size_t index = 0; index < 2; ++index)
            {
                agents.emplace_back(
                    config.seatStyles[index % config.seatStyles.size()],
                    config.handStrengths,
                    config.thresholds);
            }

            std::vector<std::reference_wrapper<IAgent>> refs;

            for (auto &agent : agents)
            {
                refs.emplace_back(agent);
            }

            runner.emplace(table, deck, std::move(refs));

            ledger.deposit("alice", 500);
            ledger.deposit("bob", 500);
            game.buyIn("alice", 500);
            game.buyIn("bob", 500);
        }

        void step(std::size_t times)
        {
            for (std::size_t index = 0; index < times; ++index)
            {
                printer.printStep(runner->step());
            }
        }

        std::chrono::system_clock::time_point time{};
        FakeClock clock{time};
        std::ostringstream out;
        Table table{2, kBlinds};
        BankrollLedger ledger;
        CashGame game{table, ledger, 100};
        std::vector<PolicyAgent> agents;
        SplitMix64Rng bits;
        Deck deck;
        TablePrinter printer{out, game, table, clock, "Test"};
        std::optional<TableRunner> runner;
        PokerSnapshotStore store{
            table, deck, bits, ledger, game, printer};
    };
} // namespace

// The whole point of the store, asserted end to end.
// A dumped room stood back up deals the remembered one's next card.
// Narration included.
TEST(SnapshotStoreTest, LoadContinuesTheSessionExactly)
{
    const antwika::testing::ScratchFile file(
        "antwika_poker_snapshot.json");

    Room original(7);
    original.step(9);
    original.store.dump(
        file.path().string(), {"> dump_state", "dumped"});

    Room resumed(1);

    const auto carried =
        resumed.store.load(file.path().string());

    EXPECT_EQ(
        carried,
        (std::vector<std::string>{"> dump_state", "dumped"}));
    EXPECT_EQ(resumed.table.remember(), original.table.remember());
    EXPECT_EQ(resumed.deck.remember(), original.deck.remember());
    EXPECT_EQ(
        resumed.bits.currentState(), original.bits.currentState());
    EXPECT_EQ(resumed.printer.remember(), original.printer.remember());
    EXPECT_EQ(resumed.ledger.balances(), original.ledger.balances());
    EXPECT_EQ(resumed.game.names(), original.game.names());

    // And the continuations agree, step for step, hands included.
    original.step(30);
    resumed.step(30);

    EXPECT_EQ(resumed.table.remember(), original.table.remember());
    EXPECT_EQ(resumed.deck.remember(), original.deck.remember());
    EXPECT_EQ(
        resumed.bits.currentState(), original.bits.currentState());
}

TEST(SnapshotStoreTest, LoadRefusesAnotherTablesCounts)
{
    const antwika::testing::ScratchFile file(
        "antwika_poker_snapshot_counts.json");

    Room original(7);
    original.store.dump(file.path().string(), {});

    Room wide(7);

    // Rewrite the dump to claim three names for a two-seat table.
    auto snapshot = [&file] {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::poker::kStateDumpMagic,
             .version = antwika::poker::kStateDumpVersion},
            "antwika poker state dump document",
            antwika::poker::standardStateDumpMigrations);
        return format.read(file.path().string());
    }();
    snapshot.state["names"].push_back("carol");

    const antwika::console::SnapshotFormat format(
        {.magic = antwika::poker::kStateDumpMagic,
         .version = antwika::poker::kStateDumpVersion},
        "antwika poker state dump document",
        antwika::poker::standardStateDumpMigrations);
    format.write(snapshot, file.path().string());

    EXPECT_THROW(
        (void)wide.store.load(file.path().string()), SnapshotError);
}

TEST(SnapshotStoreTest, LoadRefusesAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_poker_snapshot_absent.json");

    Room room(7);

    EXPECT_THROW(
        (void)room.store.load(file.path().string()), SnapshotError);
}
