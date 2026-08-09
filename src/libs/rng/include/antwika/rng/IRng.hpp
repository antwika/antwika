#pragma once

#include <cstdint>

namespace antwika::rng
{

    class IRng
    {
    public:
        virtual ~IRng() = default;

        [[nodiscard]] virtual std::uint64_t next() noexcept = 0;
    };

}
