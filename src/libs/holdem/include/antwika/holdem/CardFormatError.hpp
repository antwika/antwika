#pragma once

#include <stdexcept>

namespace antwika::holdem
{

    /**
     * @brief Thrown by parseCard()/parseCards() when text is not a card
     * in `<rank><suit>` notation, e.g. "As" or "Td".
     */
    class CardFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::holdem
