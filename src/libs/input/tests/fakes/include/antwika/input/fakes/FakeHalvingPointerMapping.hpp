#pragma once

#include "antwika/input/IPointerMapping.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input::fakes
{

    class FakeHalvingPointerMapping final : public IPointerMapping
    {
    public:
        [[nodiscard]] Position toCanvas(
            Position position) const override
        {
            return Position{.x = position.x / 2, .y = position.y / 2};
        }
    };

}
