#pragma once

#include <cstddef>
#include <span>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandValue.hpp"

namespace antwika::holdem
{

    inline constexpr std::size_t kMinEvaluatedCards = 5;

    inline constexpr std::size_t kMaxEvaluatedCards = 7;

    [[nodiscard]] HandValue evaluate(std::span<const Card> cards);

}
