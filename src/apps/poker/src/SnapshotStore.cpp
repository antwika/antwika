#include "antwika/poker/SnapshotStore.hpp"

#include <antwika/holdem/TableStateError.hpp>

namespace antwika::poker
{

    PokerSnapshotStore::PokerSnapshotStore(
        antwika::holdem::Table &table,
        antwika::holdem::Deck &deck,
        antwika::rng::SplitMix64Rng &bits,
        BankrollLedger &ledger,
        CashGame &game,
        TablePrinter &printer) noexcept
        : antwika::console::JsonSnapshotStore<StateDumpError>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika poker state dump document",
              standardStateDumpMigrations),
          table(table),
          deck(deck),
          bits(bits),
          ledger(ledger),
          game(game),
          printer(printer)
    {
    }

    nlohmann::json PokerSnapshotStore::takeState(const std::string &)
    {
        return roomDumpToJson(take());
    }

    void PokerSnapshotStore::applyState(
        const std::string &, const nlohmann::json &state)
    {
        try
        {
            apply(roomDumpFromJson(state));
        }
        // A table refusing what the codec accepted is a bad dump.
        // So it is said in this application's own error.
        // The seam then takes that to the one it promises.
        catch (const antwika::holdem::TableStateError &failed)
        {
            throw StateDumpError(failed.what());
        }
    }

    RoomDump PokerSnapshotStore::take() const
    {
        RoomDump dump;

        dump.bits = bits.currentState();
        dump.deck = deck.remember();
        dump.table = table.remember();
        dump.balances = ledger.balances();
        dump.names = game.names();
        dump.printer = printer.remember();

        return dump;

        // gcov puts the returned value's unwind block here.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    void PokerSnapshotStore::apply(const RoomDump &dump)
    {
        // The two sizes the schema cannot know are refused here.
        // A repaired dump is a session somebody never had.
        if (dump.names.size() != table.seatCount())
        {
            throw StateDumpError(
                "antwika::poker: dump seats another table's count "
                "of names");
        }

        if (dump.printer.notes.size() != table.seatCount())
        {
            throw StateDumpError(
                "antwika::poker: dump narrates another table's "
                "count of seats");
        }

        bits.restoreState(dump.bits);
        deck.restore(dump.deck);
        table.restore(dump.table, deck);
        ledger.restore(dump.balances);
        game.restoreNames(dump.names);
        printer.restore(dump.printer);
    }

} // namespace antwika::poker
