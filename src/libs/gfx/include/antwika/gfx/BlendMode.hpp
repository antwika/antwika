#pragma once

#include <cstdint>

namespace antwika::gfx
{

    enum class BlendMode : std::uint8_t
    {
        Opaque = 0,
        Alpha,
    };

    [[nodiscard]] constexpr BlendMode getLastEnumerator(BlendMode) noexcept
    {
        return BlendMode::Alpha;
    }

}
