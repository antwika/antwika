#include "antwika/poker/SnapshotStore.hpp"

#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/holdem/TableStateError.hpp>

namespace antwika::poker
{

    namespace
    {
        const antwika::console::SnapshotFormat &dumpFormat()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            static const antwika::console::SnapshotFormat format(
                {.magic = kStateDumpMagic,
                 .version = kStateDumpVersion},
                "antwika poker state dump document",
                standardStateDumpMigrations); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    PokerSnapshotStore::PokerSnapshotStore(
        antwika::holdem::Table &table,
        antwika::holdem::Deck &deck,
        antwika::rng::SplitMix64Rng &bits,
        BankrollLedger &ledger,
        CashGame &game,
        TablePrinter &printer) noexcept
        : table(table),
          deck(deck),
          bits(bits),
          ledger(ledger),
          game(game),
          printer(printer)
    {
    }

    void PokerSnapshotStore::dump(
        const std::string &path,
        const std::vector<std::string> &console)
    {
        // Built field by field, as the dump's own codec builds.
        // An aggregate temporary carries unwind blocks mid-statement.
        antwika::console::Snapshot snapshot;

        snapshot.console = console;
        snapshot.state = roomDumpToJson(take());

        dumpFormat().write(snapshot, path);
    }

    std::vector<std::string> PokerSnapshotStore::load(
        const std::string &path)
    {
        auto snapshot = dumpFormat().read(path);

        try
        {
            apply(roomDumpFromJson(snapshot.state));
        }
        // The state's own readers promise their own two errors.
        // What this seam promises is console::SnapshotError.
        catch (const StateDumpError &failed) // GCOVR_EXCL_LINE
        {
            throw antwika::console::SnapshotError(failed.what());
        }
        catch (const antwika::holdem::TableStateError &failed)
        {
            throw antwika::console::SnapshotError(failed.what());
        }

        return snapshot.console;
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
