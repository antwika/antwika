#pragma once

#include <cstdint>

namespace antwika::rng
{

    /**
     * @brief Source of reproducible pseudo-random bits.
     *
     * Deliberately not <random>: the standard engines are portable but
     * the distributions are not, and a shuffle that differs between
     * libstdc++ and libc++ would break replay determinism across the
     * project's three toolchains.
     *
     * Raw bits are the whole interface for the same reason.
     * A caller wanting a bounded draw writes the arithmetic itself, in
     * the open, where no standard library gets to choose it.
     */
    class IRng
    {
    public:
        virtual ~IRng() = default;

        /**
         * @brief Draw the next 64 pseudo-random bits.
         * @return The drawn bits, advancing the generator's state.
         */
        [[nodiscard]] virtual std::uint64_t next() noexcept = 0;
    };

} // namespace antwika::rng
