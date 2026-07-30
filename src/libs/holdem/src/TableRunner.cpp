#include "antwika/holdem/TableRunner.hpp"

#include <functional>
#include <utility>
#include <vector>

#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/StepOutcome.hpp"
#include "antwika/holdem/TableStateError.hpp"

namespace antwika::holdem
{

    TableRunner::TableRunner(
        Table &table,
        IDeck &deck,
        std::vector<std::reference_wrapper<IAgent>> agents)
        : table(table), deck(deck), agents(std::move(agents))
    {
        if (this->agents.size() != table.seatCount())
        {
            throw TableStateError(
                "TableRunner: every seat needs an agent");
        }
    }

    StepOutcome TableRunner::step()
    {
        if (!table.isHandInProgress())
        {
            if (!table.canStartHand())
            {
                return StepOutcome{
                    .kind = StepKind::TableIdle,
                    .stage = table.stage(),
                };
            }

            table.startHand(deck);

            // Two players all-in on their blinds decide nothing.
            // The hand can be over before anyone is asked anything.
            return StepOutcome{
                .kind = table.isHandInProgress()
                            ? StepKind::HandStarted
                            : StepKind::HandCompleted,
                .stage = table.stage(),
            };
        }

        // A hand in progress always has somebody to act.
        // Table runs the board out itself when nobody can wager.
        const auto actor = *table.seatToAct();
        const auto stageBefore = table.stage();
        const auto action = agents[indexOf(actor)].get().act(
            table.viewFor(actor));
        table.apply(action);

        return StepOutcome{
            .kind = table.isHandInProgress() ? StepKind::Acted
                                             : StepKind::HandCompleted,
            .prompted = true,
            .seat = actor,
            .action = action,
            .stage = table.stage(),
            .stageAdvanced = table.stage() != stageBefore,
        };
    }

} // namespace antwika::holdem
