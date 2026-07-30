#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::holdem
{

    /**
     * @brief What a player chose to do when it was their turn.
     */
    enum class ActionType : std::uint8_t
    {
        /**
         * @brief Give up the hand and any chips already committed.
         */
        Fold = 0,

        /**
         * @brief Stay in for free; legal only with nothing to call.
         */
        Check,

        /**
         * @brief Match the current bet, or go all-in trying.
         */
        Call,

        /**
         * @brief Open the betting; legal only with no bet live.
         */
        Bet,

        /**
         * @brief Increase a live bet.
         */
        Raise,
    };

    /**
     * @brief Name an action type as a player would.
     * @param type The action type to name.
     * @return A human-readable name, e.g. "raise".
     */
    [[nodiscard]] std::string_view toString(ActionType type) noexcept;

} // namespace antwika::holdem
