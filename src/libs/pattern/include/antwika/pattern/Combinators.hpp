#pragma once

#include <cstdint>
#include <vector>

#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/ParamValue.hpp"
#include "antwika/pattern/Pattern.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    [[nodiscard]] Pattern fast(Cycle factor, Pattern inner);

    [[nodiscard]] Pattern slow(Cycle factor, Pattern inner);

    [[nodiscard]] Pattern early(Cycle amount, Pattern inner);

    [[nodiscard]] Pattern late(Cycle amount, Pattern inner);

    [[nodiscard]] Pattern rev(Pattern inner);

    [[nodiscard]] Pattern euclid(
        std::int64_t pulses, std::int64_t steps, Pattern inner);

    [[nodiscard]] Pattern degradeBy(
        ParamValue chance, std::uint64_t seed, Pattern inner);

    [[nodiscard]] Pattern during(
        std::int64_t period, std::vector<Span> windows, Pattern inner);

}
