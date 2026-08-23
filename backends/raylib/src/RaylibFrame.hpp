#pragma once

#include <cstdint>

namespace antwika::raylib
{

    namespace detail
    {
        inline std::uint64_t frameCounter = 0;
    }

    [[nodiscard]] inline std::uint64_t getFrameCount() noexcept
    {
        return detail::frameCounter;
    }

    inline void advanceFrame() noexcept
    {
        ++detail::frameCounter;
    }

}
