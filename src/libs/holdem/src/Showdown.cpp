#include "Showdown.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandEvaluator.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Limits.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"

namespace antwika::holdem
{

    ShowdownScores scoreShowdown(
        std::span<const Seat> seats, std::span<const Card> board)
    {
        ShowdownScores scores{
            .values = std::vector<HandValue>(seats.size(), HandValue{}),
            .entries = {},
        };

        for (std::size_t index = 0; index < seats.size(); ++index)
        {
            const auto &seat = seats[index];
            if (!seat.inHand)
            {
                continue;
            }

            std::array<Card, kHoleCardCount + kBoardSize> cards{};
            std::copy(
                seat.holeCards.begin(), seat.holeCards.end(), cards.begin());
            std::copy(
                board.begin(),
                board.end(),
                std::next(
                    cards.begin(),
                    static_cast<std::ptrdiff_t>(kHoleCardCount)));

            const auto value = evaluate(std::span<const Card>(cards));
            scores.values[index] = value;
            scores.entries.push_back(ShowdownEntry{
                .seat = makeSeatId(index),
                .holeCards = seat.holeCards,
                .value = value,
            });
        }

        // Stable, so seats tied on strength stay in seat order.
        std::stable_sort(
            scores.entries.begin(),
            scores.entries.end(),
            [](const ShowdownEntry &left, const ShowdownEntry &right)
            { return left.value > right.value; });

        return scores;
    }

    ShowdownScores scoreWithoutShowdown(std::size_t seatCount)
    {
        return ShowdownScores{
            .values = std::vector<HandValue>(seatCount, HandValue{}),
            .entries = {},
        };
    }

} // namespace antwika::holdem
