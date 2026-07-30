#pragma once

#include <cstdint>

#include "antwika/holdem/Action.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    /**
     * @brief What one call to TableRunner::step() did.
     */
    enum class StepKind : std::uint8_t
    {
        /**
         * @brief Nothing happened: fewer than two players have chips, so
         * no hand can be dealt until somebody buys in.
         */
        TableIdle = 0,

        /**
         * @brief Blinds were posted and cards dealt.
         */
        HandStarted,

        /**
         * @brief One player was asked to act, and did.
         */
        Acted,

        /**
         * @brief One player acted, and that ended the hand.
         */
        HandCompleted,
    };

    /**
     * @brief What one step of a table's loop amounted to, in enough
     * detail to narrate it without reaching back into the table.
     */
    struct StepOutcome
    {
        /**
         * @brief What kind of step this was.
         */
        StepKind kind{};

        /**
         * @brief Whether an agent was asked for a decision.
         *
         * False for a deal, an idle table, and for the one case where a
         * hand finishes without anybody being asked anything: two blinds
         * that were all the chips their owners had. Only when this is
         * true do seat and action mean anything.
         */
        bool prompted = false;

        /**
         * @brief The seat that acted; meaningful only when prompted.
         */
        SeatId seat{};

        /**
         * @brief What that seat did; meaningful only when prompted.
         */
        Action action{};

        /**
         * @brief Chips the action moved from that seat's stack into the
         * pot; meaningful only when prompted.
         *
         * What a call actually cost, which the action itself does not
         * say: Action::amount is the round stake a Bet or Raise names,
         * and nothing at all for the other three.
         */
        Chips staked{};

        /**
         * @brief The largest stake any seat had in the betting round
         * before the action; meaningful only when prompted.
         *
         * A raise reads as "raises amount - betBefore to amount", and
         * that subtraction needs the bet the raise was measured against,
         * which the raise itself has already replaced.
         */
        Chips betBefore{};

        /**
         * @brief Whether the action put in the last chip that seat had;
         * meaningful only when prompted.
         */
        bool allIn = false;

        /**
         * @brief The stage the hand stood on after the step.
         */
        Stage stage{};

        /**
         * @brief Whether the step ended a betting round and so moved the
         * hand to a later stage.
         */
        bool stageAdvanced = false;

        bool operator==(const StepOutcome &other) const = default;
    };

} // namespace antwika::holdem
