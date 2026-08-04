#include "antwika/poker/TablePrinter.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/holdem/ActionType.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/HandResult.hpp>
#include <antwika/holdem/HandText.hpp>
#include <antwika/holdem/Limits.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/StepOutcome.hpp>

namespace antwika::poker
{

    using antwika::holdem::ActionType;
    using antwika::holdem::describe;
    using antwika::holdem::indexOf;
    using antwika::holdem::kBoardSize;
    using antwika::holdem::kFlopSize;
    using antwika::holdem::kMinSeats;
    using antwika::holdem::makeSeatId;
    using antwika::holdem::StepKind;
    using antwika::holdem::toString;

    namespace
    {

        [[nodiscard]] std::string titled(std::string_view text)
        {
            std::string name(text);
            name[0] = static_cast<char>(
                std::toupper(static_cast<unsigned char>(name[0])));
            return name;

            // Unwinding is the only way to reach the brace below.
            // The local string's destructor is all that lives there.
        } // GCOVR_EXCL_LINE

    } // namespace

    TablePrinter::TablePrinter(
        std::ostream &out,
        const CashGame &game,
        const Table &table,
        IClock &clock,
        std::string tableName)
        : out(out),
          game(game),
          table(table),
          clock(clock),
          tableName(std::move(tableName)),
          notes(table.seatCount())
    {
    }

    void TablePrinter::printStep(const StepOutcome &outcome)
    {
        if (outcome.kind == StepKind::TableIdle)
        {
            return;
        }
        if (outcome.kind == StepKind::HandStarted)
        {
            printHandStart();
            return;
        }
        if (outcome.kind == StepKind::Acted)
        {
            printAction(outcome);
            if (outcome.stageAdvanced)
            {
                printStreets(table.board());
                for (auto &note : notes)
                {
                    note.roundStake = 0;
                }
                stage = outcome.stage;
            }
            return;
        }

        // Both blinds can be all-in the moment they are posted.
        // Then the deal is the whole hand.
        // There was no earlier step to have written its start up in.
        if (outcome.prompted)
        {
            printAction(outcome);
        }
        else
        {
            printHandStart();
        }
        printResult();
    }

    void TablePrinter::printHandStart()
    {
        boardShown = 0;
        stage = Stage::PreFlop;
        smallBlindSeat.reset();
        bigBlindSeat.reset();
        for (auto &note : notes)
        {
            note = PrinterNote{};
        }

        const auto blinds = table.blinds();
        out << "\nAntwika Hand #" << table.handsPlayed()
            << ": Hold'em No Limit (" << blinds.small << '/' << blinds.big
            << ") - " << timestamp() << '\n';
        out << "Table '" << tableName << "' " << table.seatCount()
            << "-max Seat #" << (indexOf(table.button()) + 1)
            << " is the button\n";

        for (std::size_t index = 0; index < table.seatCount(); ++index)
        {
            const auto seat = makeSeatId(index);
            if (!game.playerAt(seat))
            {
                continue;
            }
            notes[index].dealtIn = wasDealtIn(seat);
            notes[index].roundStake = table.seatAt(seat).committed;
            out << seatLabel(seat) << " (" << stackBeforeTheHand(seat)
                << " in chips)\n";
        }

        printBlinds();

        out << "*** HOLE CARDS ***\n";
        for (std::size_t index = 0; index < table.seatCount(); ++index)
        {
            if (!notes[index].dealtIn)
            {
                continue;
            }
            const auto seat = makeSeatId(index);
            out << "Dealt to " << nameOf(seat) << " ["
                << toString(table.seatAt(seat).holeCards) << "]\n";
        }
    }

    void TablePrinter::printBlinds()
    {
        const auto dealtIn = std::ranges::count_if(
            notes, [](const PrinterNote &note) { return note.dealtIn; });

        // Heads-up the button posts the small blind, as Table has it.
        // Everywhere else the blinds sit to the left of the button.
        smallBlindSeat =
            dealtIn == static_cast<std::ptrdiff_t>(kMinSeats)
                ? std::optional<SeatId>(table.button())
                : nextInHand(table.button());
        if (!smallBlindSeat)
        {
            return;
        }

        bigBlindSeat = nextInHand(*smallBlindSeat);
        printPost("small blind", *smallBlindSeat);
        printPost("big blind", *bigBlindSeat);
    }

    // The round's stake is no good here.
    // A hand that ran itself out has reset it several times over.
    // A blind is still the whole of what that seat committed.
    void TablePrinter::printPost(std::string_view blind, SeatId seat)
    {
        const auto posted = table.seatAt(seat).committed;
        out << nameOf(seat) << ": posts " << blind << ' ' << posted;
        if (stackBeforeTheHand(seat) == posted)
        {
            out << " and is all-in";
        }
        out << '\n';
    }

    void TablePrinter::printAction(const StepOutcome &outcome)
    {
        auto &note = notes[indexOf(outcome.seat)];
        note.roundStake += outcome.staked;

        out << nameOf(outcome.seat) << ": ";
        const auto type = outcome.action.type;
        if (type == ActionType::Fold)
        {
            note.folded = true;
            note.foldedOn = stage;
            out << "folds";
        }
        else if (type == ActionType::Check)
        {
            out << "checks";
        }
        else if (type == ActionType::Call)
        {
            out << "calls " << outcome.staked;
        }
        else if (type == ActionType::Bet)
        {
            out << "bets " << outcome.action.amount;
        }
        else
        {
            out << "raises " << outcome.action.amount - outcome.betBefore
                << " to " << outcome.action.amount;
        }

        if (outcome.allIn)
        {
            out << " and is all-in";
        }
        out << '\n';
    }

    void TablePrinter::printStreets(std::span<const Card> board)
    {
        if (boardShown < kFlopSize && board.size() >= kFlopSize)
        {
            out << "*** FLOP *** [" << toString(board.first(kFlopSize))
                << "]\n";
            boardShown = kFlopSize;
        }
        if (boardShown < kFlopSize + 1 && board.size() >= kFlopSize + 1)
        {
            out << "*** TURN *** [" << toString(board.first(kFlopSize))
                << "] [" << toString(board[kFlopSize]) << "]\n";
            boardShown = kFlopSize + 1;
        }
        if (boardShown < kBoardSize && board.size() >= kBoardSize)
        {
            out << "*** RIVER *** ["
                << toString(board.first(kBoardSize - 1)) << "] ["
                << toString(board[kBoardSize - 1]) << "]\n";
            boardShown = kBoardSize;
        }
    }

    void TablePrinter::printResult()
    {
        const auto &result = table.lastResult();
        const auto returned = uncalledBet();
        if (returned.amount > 0)
        {
            out << "Uncalled bet (" << returned.amount
                << ") returned to " << nameOf(returned.seat) << '\n';
        }

        printStreets(result.board);

        if (!result.showdown.empty())
        {
            out << "*** SHOW DOWN ***\n";
            for (const auto &entry : result.showdown)
            {
                out << nameOf(entry.seat) << ": shows ["
                    << toString(entry.holeCards) << "] ("
                    << describe(entry.value) << ")\n";
            }
        }

        for (const auto &payout : result.payouts)
        {
            // A returned bet was collected from nobody.
            // The line above already said where it went.
            const auto collected = collectedBy(payout.seat, returned);
            if (collected == 0)
            {
                continue;
            }
            out << nameOf(payout.seat) << " collected " << collected
                << " from pot\n";
        }

        printSummary(result, returned);
    }

    void TablePrinter::printSummary(
        const HandResult &result, Returned returned)
    {
        out << "*** SUMMARY ***\n";
        out << "Total pot " << result.pot - returned.amount
            << " | Rake 0\n";
        if (!result.board.empty())
        {
            out << "Board [" << toString(result.board) << "]\n";
        }

        for (std::size_t index = 0; index < notes.size(); ++index)
        {
            if (!notes[index].dealtIn)
            {
                continue;
            }
            const auto seat = makeSeatId(index);
            out << seatLabel(seat) << positionsOf(seat) << ' '
                << outcomeOf(seat, returned) << '\n';
        }
    }

    // A bet nobody covered is the top stake of the round.
    // Only the part above the next stake down was ever at risk.
    TablePrinter::Returned TablePrinter::uncalledBet() const
    {
        Returned top;
        Chips second = 0;
        for (std::size_t index = 0; index < notes.size(); ++index)
        {
            const auto stake = notes[index].roundStake;
            if (stake > top.amount)
            {
                second = top.amount;
                top = Returned{
                    .seat = makeSeatId(index),
                    .amount = stake,
                };
                continue;
            }
            if (stake > second)
            {
                second = stake;
            }
        }

        return Returned{.seat = top.seat, .amount = top.amount - second};
    }

    Chips TablePrinter::collectedBy(SeatId seat, Returned returned) const
    {
        Chips collected = 0;
        for (const auto &payout : table.lastResult().payouts)
        {
            if (payout.seat == seat)
            {
                collected = payout.amount;
            }
        }

        // An uncalled bet rides back out with the pot's top layer.
        // Nobody put chips in against it, so nobody collected it.
        if (returned.seat == seat)
        {
            collected -= returned.amount;
        }
        return collected;
    }

    std::string TablePrinter::outcomeOf(
        SeatId seat, Returned returned) const
    {
        const auto &note = notes[indexOf(seat)];
        if (note.folded)
        {
            if (note.foldedOn == Stage::PreFlop)
            {
                return "folded before Flop";
            }
            return "folded on the " + titled(toString(note.foldedOn));
        }

        const auto collected = collectedBy(seat, returned);
        const auto amount = std::to_string(collected);
        const auto &showdown = table.lastResult().showdown;
        const auto shown = std::ranges::find_if(
            showdown,
            [seat](const auto &entry) { return entry.seat == seat; });
        if (shown == showdown.end())
        {
            return "collected (" + amount + ")";
        }

        const auto cards =
            "showed [" + toString(shown->holeCards) + "] and ";
        if (collected > 0)
        {
            return cards + "won (" + amount + ") with "
                   + describe(shown->value);
        }
        return cards + "lost with " + describe(shown->value);
    }

    std::string TablePrinter::timestamp() const
    {
        return std::format(
            "{:%Y/%m/%d %H:%M:%S}",
            std::chrono::floor<std::chrono::seconds>(clock.now()));
    }

    std::string TablePrinter::nameOf(SeatId seat) const
    {
        const auto player = game.playerAt(seat);
        if (player)
        {
            return *player;
        }
        return "Seat " + std::to_string(indexOf(seat) + 1);
    }

    std::string TablePrinter::seatLabel(SeatId seat) const
    {
        return "Seat " + std::to_string(indexOf(seat) + 1) + ": "
               + nameOf(seat);
    }

    std::string TablePrinter::positionsOf(SeatId seat) const
    {
        std::string text;
        if (seat == table.button())
        {
            text += " (button)";
        }
        if (smallBlindSeat == seat)
        {
            text += " (small blind)";
        }
        if (bigBlindSeat == seat)
        {
            text += " (big blind)";
        }
        return text;

        // Unwinding is the only way to reach the brace below.
        // The local string's destructor is all that lives there.
    } // GCOVR_EXCL_LINE

    std::optional<SeatId> TablePrinter::nextInHand(SeatId from) const
    {
        for (std::size_t step = 1; step <= notes.size(); ++step)
        {
            const auto index = (indexOf(from) + step) % notes.size();
            if (notes[index].dealtIn)
            {
                return makeSeatId(index);
            }
        }
        return std::nullopt;
    }

    // A table between hands has a result, once it has dealt one.
    bool TablePrinter::handJustEnded() const
    {
        return !table.isHandInProgress() && table.handsPlayed() > 0;
    }

    bool TablePrinter::wasDealtIn(SeatId seat) const
    {
        if (!handJustEnded())
        {
            return table.seatAt(seat).inHand;
        }

        // The hand was over before anybody was asked anything.
        // Whoever turned cards over is who it had been dealt to.
        const auto &showdown = table.lastResult().showdown;
        return std::ranges::any_of(
            showdown,
            [seat](const auto &entry) { return entry.seat == seat; });
    }

    Chips TablePrinter::stackBeforeTheHand(SeatId seat) const
    {
        const auto &state = table.seatAt(seat);
        auto stack = state.stack + state.committed;
        if (!handJustEnded())
        {
            return stack;
        }

        // A finished hand has already been paid out into the stack.
        // Those chips were won here rather than brought here.
        for (const auto &payout : table.lastResult().payouts)
        {
            if (payout.seat == seat)
            {
                stack -= payout.amount;
            }
        }
        return stack;
    }

    PrinterMemory TablePrinter::remember() const
    {
        // Handing the notes to the memory copies one vector.
        // The edge gcov reports here is its unwind path only.
        return PrinterMemory{ // GCOVR_EXCL_LINE
            .notes = notes,
            .smallBlindSeat = smallBlindSeat,
            .bigBlindSeat = bigBlindSeat,
            .stage = stage,
            .boardShown = boardShown};

        // gcov puts the returned value's unwind block here.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    void TablePrinter::restore(const PrinterMemory &memory)
    {
        notes = memory.notes;
        smallBlindSeat = memory.smallBlindSeat;
        bigBlindSeat = memory.bigBlindSeat;
        stage = memory.stage;
        boardShown = memory.boardShown;
    }

} // namespace antwika::poker
