#pragma once

#include <cstdint>
#include <map>
#include <string>

#include <antwika/holdem/Chips.hpp>

namespace antwika::poker
{

    using antwika::holdem::Chips;

    /**
     * @brief How a session of poker turned out.
     */
    struct RoomSummary
    {
        /**
         * @brief Hands dealt over the session.
         */
        std::uint64_t handsPlayed = 0;

        /**
         * @brief Every player's bankroll at the end, in name order,
         * after everyone who could has cashed out.
         */
        std::map<std::string, Chips> balances;

        /**
         * @brief Chips still sitting at the table.
         *
         * Non-zero only when the session was stopped part-way through a
         * hand, which is chips nobody has won yet -- reported rather
         * than folded into a bankroll, so the books still balance.
         */
        Chips chipsLeftOnTable = 0;

        bool operator==(const RoomSummary &other) const = default;
    };

} // namespace antwika::poker
