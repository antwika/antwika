#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "antwika/holdem/Card.hpp"

namespace antwika::holdem
{

    [[nodiscard]] std::string toString(Card card);

    [[nodiscard]] std::string toString(std::span<const Card> cards);

    [[nodiscard]] Card parseCard(std::string_view text);

    [[nodiscard]] std::vector<Card> parseCards(std::string_view text);

}
