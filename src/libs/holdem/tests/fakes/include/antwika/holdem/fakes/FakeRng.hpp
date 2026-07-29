#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <antwika/holdem/IRng.hpp>

namespace antwika::holdem::fakes
{

    using antwika::holdem::IRng;

    /**
     * @brief IRng that hands back a scripted sequence, cycling once it
     * reaches the end.
     */
    class FakeRng final : public IRng
    {
    public:
        /**
         * @brief Construct the generator over the values to hand back.
         * @param values The sequence to draw from; must not be empty.
         */
        explicit FakeRng(std::vector<std::uint64_t> values)
            : values(std::move(values))
        {
        }

        /**
         * @brief Draw the next scripted value.
         * @return That value, wrapping to the start when exhausted.
         */
        [[nodiscard]] std::uint64_t next() noexcept override
        {
            const auto value = values[position];
            position = (position + 1) % values.size();
            return value;
        }

    private:
        std::vector<std::uint64_t> values;
        std::size_t position = 0;
    };

} // namespace antwika::holdem::fakes
