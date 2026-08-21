#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <antwika/rng/IRng.hpp>

namespace antwika::rng::fakes
{

    using antwika::rng::IRng;

    class FakeRng final : public IRng
    {
    public:
        explicit FakeRng(std::vector<std::uint64_t> values)
            : values(std::move(values))
        {
        }

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

}
