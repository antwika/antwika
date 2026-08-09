#pragma once

#include <nlohmann/json.hpp>

#include <string>

#include <antwika/console/IJsonSnapshotStore.hpp>
#include <antwika/holdem/Deck.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/rng/SplitMix64Rng.hpp>

#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/StateDump.hpp"
#include "antwika/poker/TablePrinter.hpp"

namespace antwika::poker
{

    class PokerSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<StateDumpError>
    {
    public:
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

}
