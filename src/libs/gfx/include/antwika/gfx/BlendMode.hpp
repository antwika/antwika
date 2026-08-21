#pragma once

#include <cstdint>

namespace antwika::gfx
{

    enum class BlendMode : std::uint8_t
    {
        Opaque = 0,
        Alpha,
    };

    [[nodiscard]] constexpr BlendMode lastEnumerator(BlendMode) noexcept
    {
        return BlendMode::Alpha;
    }

}
