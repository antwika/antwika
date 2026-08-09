#pragma once

#include <cstdint>

namespace antwika::raylib
{

    namespace detail
    {
        inline std::uint64_t presented = 0;
    }

    [[nodiscard]] inline std::uint64_t frameCount() noexcept
    {
        return detail::presented;
    }

    inline void advanceFrame() noexcept
    {
        ++detail::presented;
    }

}
