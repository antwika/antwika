#pragma once

#include "antwika/holdem/ActionType.hpp"
#include "antwika/holdem/Chips.hpp"

namespace antwika::holdem
{

    /**
     * @brief One player's decision, as handed back to the table.
     */
    struct Action
    {
        /**
         * @brief What the player chose to do.
         */
        ActionType type{};

        /**
         * @brief For Bet and Raise, the total this player's stake in the
         * current round becomes -- not the increment added to it.
         *
         * "Raise to 200" is unambiguous where "raise 200" is not, and it
         * is also the number a table needs, so the ambiguity is removed
         * here rather than in each caller. Ignored for Fold, Check and
         * Call, whose amount is fully determined by the table.
         */
        Chips amount{};

        bool operator==(const Action &other) const = default;
    };

    /**
     * @brief Build a fold.
     * @return The action.
     */
    [[nodiscard]] constexpr Action fold() noexcept
    {
        return Action{.type = ActionType::Fold, .amount = 0};
    }

    /**
     * @brief Build a check.
     * @return The action.
     */
    [[nodiscard]] constexpr Action check() noexcept
    {
        return Action{.type = ActionType::Check, .amount = 0};
    }

    /**
     * @brief Build a call.
     * @return The action.
     */
    [[nodiscard]] constexpr Action call() noexcept
    {
        return Action{.type = ActionType::Call, .amount = 0};
    }

    /**
     * @brief Build a bet.
     * @param amount The stake this round becomes.
     * @return The action.
     */
    [[nodiscard]] constexpr Action bet(Chips amount) noexcept
    {
        return Action{.type = ActionType::Bet, .amount = amount};
    }

    /**
     * @brief Build a raise.
     * @param amount The stake this round becomes.
     * @return The action.
     */
    [[nodiscard]] constexpr Action raiseTo(Chips amount) noexcept
    {
        return Action{.type = ActionType::Raise, .amount = amount};
    }

} // namespace antwika::holdem
