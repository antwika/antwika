#pragma once

#include "antwika/holdem/Card.hpp"

namespace antwika::holdem
{

    class IDeck
    {
    public:
        virtual ~IDeck() = default;

        virtual void shuffle() = 0;

        [[nodiscard]] virtual Card deal() = 0;
    };

}
