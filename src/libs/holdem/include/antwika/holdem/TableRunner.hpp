#pragma once

#include <functional>
#include <vector>

#include "antwika/holdem/IAgent.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/StepOutcome.hpp"
#include "antwika/holdem/Table.hpp"

namespace antwika::holdem
{

    /**
     * @brief The poker loop: deal a hand, ask whoever is to act what
     * they want to do, apply it, and start the next hand when the last
     * one is paid out.
     *
     * Kept apart from Table so the rules never depend on there being
     * agents at all: Table answers "what is legal", this answers "whose
     * opinion do we need next". One step() is exactly one of those
     * questions, which is what lets a caller drive a hand of poker from
     * an outer loop it already has -- a fixed simulation tick, in this
     * project's case -- instead of surrendering control to a loop in
     * here.
     */
    class TableRunner final
    {
    public:
        /**
         * @brief Construct the loop over its table, deck and players.
         * @param table The table whose rules are being followed.
         * @param deck Shuffled and dealt from at the start of each hand.
         * @param agents One agent per seat, indexed by seat id; a seat
         * nobody sits in still needs an entry, which is never asked
         * anything.
         * @throws TableStateError If agents does not cover every seat.
         */
        TableRunner(
            Table &table,
            IDeck &deck,
            std::vector<std::reference_wrapper<IAgent>> agents);

        TableRunner(const TableRunner &) = delete;
        TableRunner(TableRunner &&) = delete;

        TableRunner &operator=(const TableRunner &) = delete;
        TableRunner &operator=(TableRunner &&) = delete;

        /**
         * @brief Advance the table by one decision, or by one deal.
         *
         * Deliberately does at most one thing, so a caller can pace a
         * game of poker however it likes and see every action as it
         * happens rather than being handed a finished hand.
         * @return What the step did.
         * @throws IllegalActionError If the asked agent returned an
         * action that breaks the betting rules.
         */
        StepOutcome step();

    private:
        Table &table;
        IDeck &deck;
        std::vector<std::reference_wrapper<IAgent>> agents;
    };

} // namespace antwika::holdem
