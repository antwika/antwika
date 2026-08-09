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

            return StepOutcome{
                .kind = table.isHandInProgress()
                            ? StepKind::HandStarted
                            : StepKind::HandCompleted,
                .stage = table.stage(),
            };
        }

        const auto actor = *table.seatToAct();
        const auto stageBefore = table.stage();
        const auto view = table.viewFor(actor);
        const auto committedBefore = table.seatAt(actor).committed;
        const auto action = agents[indexOf(actor)].get().act(view);
        table.apply(action);

        const auto staked = table.seatAt(actor).committed - committedBefore;

        return StepOutcome{
            .kind = table.isHandInProgress() ? StepKind::Acted
                                             : StepKind::HandCompleted,
            .prompted = true,
            .seat = actor,
            .action = action,
            .staked = staked,
            .betBefore = view.currentBet,
            .allIn = staked == view.stack,
            .stage = table.stage(),
            .stageAdvanced = table.stage() != stageBefore,
        };
    }

}
