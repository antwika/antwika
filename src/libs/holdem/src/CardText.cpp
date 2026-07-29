#include "antwika/holdem/CardText.hpp"

#include <cctype>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/CardFormatError.hpp"

namespace antwika::holdem
{

    namespace
    {

        constexpr std::string_view kRankChars = "23456789TJQKA";
        constexpr std::string_view kSuitChars = "cdhs";
        constexpr std::size_t kCardTextLength = 2;

        [[nodiscard]] char toLower(char value) noexcept
        {
            return static_cast<char>(
                std::tolower(static_cast<unsigned char>(value)));
        }

    } // namespace

    std::string toString(Card card)
    {
        return std::string{
            kRankChars[rawValue(rankOf(card))],
            kSuitChars[rawValue(suitOf(card))]};
    }

    std::string toString(std::span<const Card> cards)
    {
        std::string text;
        for (const auto card : cards)
        {
            if (!text.empty())
            {
                text.push_back(' ');
            }
            text += toString(card);
        }
        return text;
    } // GCOVR_EXCL_LINE

    Card parseCard(std::string_view text)
    {
        if (text.size() != kCardTextLength)
        {
            throw CardFormatError(
                "parseCard: a card is written as two characters, got \""
                + std::string(text) + "\"");
        }

        const auto rankIndex =
            kRankChars.find(static_cast<char>(std::toupper(
                static_cast<unsigned char>(text[0]))));
        if (rankIndex == std::string_view::npos)
        {
            throw CardFormatError(
                "parseCard: \"" + std::string(text)
                + "\" names no rank");
        }

        const auto suitIndex = kSuitChars.find(toLower(text[1]));
        if (suitIndex == std::string_view::npos)
        {
            throw CardFormatError(
                "parseCard: \"" + std::string(text)
                + "\" names no suit");
        }

        return makeCard(
            static_cast<Rank>(rankIndex), static_cast<Suit>(suitIndex));
    }

    std::vector<Card> parseCards(std::string_view text)
    {
        std::istringstream stream{std::string(text)};
        std::vector<Card> cards;
        std::string token;
        while (stream >> token)
        {
            cards.push_back(parseCard(token));
        }
        return cards;
    }

} // namespace antwika::holdem
