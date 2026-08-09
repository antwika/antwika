#pragma once

#include "antwika/input/Position.hpp"

namespace antwika::input
{

    class IPointerMapping
    {
    public:
        virtual ~IPointerMapping() = default;

        [[nodiscard]] virtual Position toSurface(
            Position position) const = 0;
    };

}
