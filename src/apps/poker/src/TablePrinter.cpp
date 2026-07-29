#include "antwika/poker/TablePrinter.hpp"

#include <ostream>
#include <string>

#include <antwika/holdem/ActionType.hpp>
#include <antwika/holdem/CardText.hpp>
#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/HandValue.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/StepOutcome.hpp>

namespace antwika::poker
{

    using antwika::holdem::ActionType;
    using antwika::holdem::categoryOf;
    using antwika::holdem::SeatId;
    using antwika::holdem::StepKind;
    using antwika::holdem::toString;

    TablePrinter::TablePrinter(
        std::ostream &out, const CashGame &game, const Table &table)
        : out(out), game(game), table(table)
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
            return;
        }

        // Both blinds can be all-in the moment they are posted.
        // A hand settled by the deal alone has no action to report.
        if (outcome.prompted)
        {
            printAction(outcome);
        }
        printResult();
    }

    void TablePrinter::printHandStart()
    {
        out << "\n--- hand " << table.handsPlayed() << ", button on "
            << nameOf(table.button()) << " ---\n";
        for (std::size_t index = 0; index < table.seatCount(); ++index)
        {
            const auto seat = antwika::holdem::makeSeatId(index);
            if (!game.playerAt(seat))
            {
                continue;
            }
            out << "  " << nameOf(seat) << ": "
                << table.seatAt(seat).stack << " chips";
            if (table.seatAt(seat).inHand)
            {
                out << ", dealt "
                    << toString(table.seatAt(seat).holeCards);
            }
            out << '\n';
        }
    }

    void TablePrinter::printAction(const StepOutcome &outcome)
    {
        out << "  " << nameOf(outcome.seat) << " "
            << toString(outcome.action.type);
        if (outcome.action.type == ActionType::Bet)
        {
            // Nothing is staked this round or there would be a raise.
            // So the total and the increment are the same number.
            // "bet 40" is the plainer way to say it.
            out << " " << outcome.action.amount;
        }
        else if (outcome.action.type == ActionType::Raise)
        {
            out << " to " << outcome.action.amount;
        }

        // The result line reports the final pot of a finished hand.
        // So a running total here would only repeat it.
        if (table.isHandInProgress())
        {
            out << " (pot " << table.pot() << ")";
        }
        out << '\n';

        // A stage change into the showdown needs no line of its own.
        // printResult() is about to say the same thing and more.
        if (outcome.stageAdvanced
            && outcome.stage != antwika::holdem::Stage::Showdown)
        {
            out << "  -- " << toString(outcome.stage) << ": "
                << toString(table.board()) << '\n';
        }
    }

    void TablePrinter::printResult()
    {
        const auto &result = table.lastResult();
        out << "  -- " << toString(result.stage);
        if (!result.board.empty())
        {
            out << ", board " << toString(result.board);
        }
        out << ", pot " << result.pot << '\n';
        for (const auto &entry : result.showdown)
        {
            out << "  " << nameOf(entry.seat) << " shows "
                << toString(entry.holeCards) << " for "
                << toString(categoryOf(entry.value)) << '\n';
        }
        for (const auto &payout : result.payouts)
        {
            out << "  " << nameOf(payout.seat) << " wins "
                << payout.amount << '\n';
        }
    }

    std::string TablePrinter::nameOf(SeatId seat) const
    {
        const auto player = game.playerAt(seat);
        if (player)
        {
            return *player;
        }
        return "seat " + std::to_string(antwika::holdem::rawValue(seat));
    }

} // namespace antwika::poker
