#pragma once

#include <string>
#include <vector>

#include <antwika/console/ISnapshotStore.hpp>
#include <antwika/holdem/Deck.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/rng/SplitMix64Rng.hpp>

#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/StateDump.hpp"
#include "antwika/poker/TablePrinter.hpp"

namespace antwika::poker
{

    /**
     * @brief This application's half of dump_state and load_state.
     *
     * console::SnapshotCommands owns the policy; this owns what the
     * state *is*: the table, the deck, the generator's counter, the
     * bankrolls, the seating and the hand history's narration, taken
     * and applied together so a resumed session deals the very next
     * card the remembered one would have.
     */
    class PokerSnapshotStore final : public antwika::console::ISnapshotStore
    {
    public:
        /**
         * @brief Construct the store over the live room.
         * @param table The table a dump remembers mid-hand. Must
         * outlive this store.
         * @param deck The deck, restored to its exact position. Must
         * outlive this store.
         * @param bits The generator the deck shuffles from. Must
         * outlive this store.
         * @param ledger Every bankroll. Must outlive this store.
         * @param game The seating. Must outlive this store.
         * @param printer The hand history's narration. Must outlive
         * this store.
         */
        PokerSnapshotStore(
            antwika::holdem::Table &table,
            antwika::holdem::Deck &deck,
            antwika::rng::SplitMix64Rng &bits,
            BankrollLedger &ledger,
            CashGame &game,
            TablePrinter &printer) noexcept;

        PokerSnapshotStore(const PokerSnapshotStore &) = delete;
        PokerSnapshotStore(PokerSnapshotStore &&) = delete;

        PokerSnapshotStore &operator=(const PokerSnapshotStore &) =
            delete;
        PokerSnapshotStore &operator=(PokerSnapshotStore &&) = delete;

        /**
         * @brief Write the running state to a file.
         * @param path Where to write it.
         * @param console The console's history, carried in the dump.
         * @throws console::SnapshotError If the file cannot be
         * written.
         */
        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override;

        /**
         * @brief Read a file and apply the state it holds.
         * @param path The file to read.
         * @return The console history the dump carried.
         * @throws console::SnapshotError If the file is not there, is
         * not this application's dump, or cannot be applied.
         */
        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) override;

    private:
        [[nodiscard]] RoomDump take() const;

        void apply(const RoomDump &dump);

        antwika::holdem::Table &table;
        antwika::holdem::Deck &deck;
        antwika::rng::SplitMix64Rng &bits;
        BankrollLedger &ledger;
        CashGame &game;
        TablePrinter &printer;
    };

} // namespace antwika::poker
