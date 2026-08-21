#pragma once

#include <cstdint>

#include "antwika/rng/IRng.hpp"

namespace antwika::rng
{

    class SplitMix64Rng final : public IRng
    {
    public:
        explicit SplitMix64Rng(std::uint64_t seed) noexcept;

        [[nodiscard]] std::uint64_t next() noexcept override;

        [[nodiscard]] std::uint64_t currentState() const noexcept;

        void restoreState(std::uint64_t value) noexcept;

    private:
        std::uint64_t state;
    };

}
