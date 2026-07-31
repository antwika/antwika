#pragma once

#include <cstdint>

#include "antwika/rng/IRng.hpp"

namespace antwika::rng
{

    /**
     * @brief IRng implementing splitmix64, a fixed sequence of shifts,
     * xors and multiplies over a 64-bit counter.
     *
     * Every operation is defined on exact-width unsigned integers, so
     * the same seed yields the same stream on every platform the project
     * builds for -- which is what a recorded hand of poker needs in
     * order to deal itself the same way on replay.
     *
     * The sequence is part of the contract rather than an implementation
     * detail, since recorded sessions and checked-in demo replays are
     * reproduced from it.
     */
    class SplitMix64Rng final : public IRng
    {
    public:
        /**
         * @brief Construct the generator over its starting state.
         * @param seed Initial state; any value, including zero, is fine.
         */
        explicit SplitMix64Rng(std::uint64_t seed) noexcept;

        /**
         * @brief Draw the next 64 pseudo-random bits.
         * @return The drawn bits, advancing the generator's state.
         */
        [[nodiscard]] std::uint64_t next() noexcept override;

    private:
        std::uint64_t state;
    };

} // namespace antwika::rng
