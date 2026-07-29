#pragma once

/**
 * @file
 * @brief Names of events defined by this application.
 */
namespace antwika::poker::events
{

    /**
     * @brief Puts money into a player's bankroll, outside any game.
     *
     * The payload is a JSON object with fields "player" (a name) and
     * "amount" (a positive integer). This is the only way chips enter
     * the room: a player's bankroll is the ceiling on what they can
     * ever bring to a table, so nothing else may create it.
     */
    inline constexpr const char *kDeposit = "poker.deposit";

    /**
     * @brief Moves chips from a player's bankroll onto a table seat.
     *
     * The payload is a JSON object with fields "player" and "amount".
     * A buy-in larger than the player's bankroll is refused outright,
     * and a player already seated is topped up in place rather than
     * given a second seat.
     */
    inline constexpr const char *kBuyIn = "poker.buy_in";

    /**
     * @brief Takes a player's chips off the table and back into their
     * bankroll.
     *
     * The payload is a JSON object with a single "player" field.
     */
    inline constexpr const char *kCashOut = "poker.cash_out";

} // namespace antwika::poker::events
