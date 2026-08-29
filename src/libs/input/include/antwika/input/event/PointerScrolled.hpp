#pragma once

#include <cstdint>

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    struct PointerScrolled final
    {
        std::int32_t horizontal = 0;
        std::int32_t vertical = 0;

        [[nodiscard]] bool operator==(
            const PointerScrolled &other) const = default;
    };

}
