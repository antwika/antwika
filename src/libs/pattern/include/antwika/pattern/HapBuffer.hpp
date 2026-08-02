#pragma once

#include <cstddef>
#include <vector>

#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/IHapSink.hpp"

namespace antwika::pattern
{

    /**
     * @brief Collects what a query found, in the order it arrived.
     *
     * The sink almost every caller wants, and the one every test wants:
     * a whole cycle of a pattern comes back as a vector and is one
     * EXPECT_EQ against a literal list.
     *
     * It is in the library rather than under `tests/` for the reason
     * antwika::sound's null backend is: a caller that reserves once and
     * clears each tick has no allocation on its hot path, and that
     * caller is a sequencer rather than a test.
     */
    class HapBuffer final : public IHapSink
    {
    public:
        /**
         * @brief Take one event.
         * @param hap The event to keep.
         */
        void accept(const Hap &hap) override;

        /**
         * @brief Get what has been collected.
         * @return The events, in the order they arrived.
         */
        [[nodiscard]] const std::vector<Hap> &haps() const noexcept;

        /**
         * @brief Forget everything collected, keeping the space.
         */
        void clear() noexcept;

        /**
         * @brief Make room for a number of events up front.
         * @param count How many to make room for.
         */
        void reserve(std::size_t count);

    private:
        std::vector<Hap> collected;
    };

} // namespace antwika::pattern
