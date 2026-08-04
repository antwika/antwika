#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>
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
     *
     * A dump the codec accepted and the table itself refuses is said
     * in StateDumpError, this application's own category, so it leaves
     * the seam as the console::SnapshotError every refusal does.
     */
    class PokerSnapshotStore final
        : public antwika::console::JsonSnapshotStore<StateDumpError>
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

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

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
