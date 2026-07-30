#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "antwika/holdem/Card.hpp"

namespace antwika::holdem
{

    /**
     * @brief Render a card in `<rank><suit>` notation.
     * @param card The card to render.
     * @return Two characters, e.g. "As" or "Td".
     */
    [[nodiscard]] std::string toString(Card card);

    /**
     * @brief Render a run of cards space-separated.
     * @param cards The cards to render, in the given order.
     * @return The rendered cards, e.g. "As Td 7c", empty if none.
     */
    [[nodiscard]] std::string toString(std::span<const Card> cards);

    /**
     * @brief Parse one card from `<rank><suit>` notation.
     *
     * Ranks are `23456789TJQKA` and suits are `cdhs`, both
     * case-insensitive.
     * @param text Exactly two characters naming a card.
     * @return The parsed card.
     * @throws CardFormatError If the text is not two characters, or
     * either character names no rank or suit.
     */
    [[nodiscard]] Card parseCard(std::string_view text);

    /**
     * @brief Parse a whitespace-separated list of cards.
     * @param text The cards to parse, e.g. "As Td 7c".
     * @return The parsed cards, in the order given.
     * @throws CardFormatError If any entry is not a valid card.
     */
    [[nodiscard]] std::vector<Card> parseCards(std::string_view text);

} // namespace antwika::holdem
