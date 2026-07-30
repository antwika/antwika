#pragma once

#include <cstddef>

/**
 * @file
 * @brief Fixed structural sizes of a Texas hold'em hand.
 */
namespace antwika::holdem
{

    /**
     * @brief Cards in a made poker hand, the five that get compared.
     */
    inline constexpr std::size_t kHandSize = 5;

    /**
     * @brief Private cards dealt to each player in the hand.
     */
    inline constexpr std::size_t kHoleCardCount = 2;

    /**
     * @brief Community cards on the board once the river is out.
     */
    inline constexpr std::size_t kBoardSize = 5;

    /**
     * @brief Community cards turned over when moving to the flop.
     */
    inline constexpr std::size_t kFlopSize = 3;

    /**
     * @brief Fewest seated players a hand can be dealt to.
     */
    inline constexpr std::size_t kMinSeats = 2;

    /**
     * @brief Most players one table seats.
     */
    inline constexpr std::size_t kMaxSeats = 9;

} // namespace antwika::holdem
