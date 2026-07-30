#pragma once

#include <cstdint>

namespace antwika::holdem
{

    /**
     * @brief An indivisible amount of betting currency.
     *
     * A plain integer alias rather than a scoped enum, following
     * antwika::time::Tick: chips are arithmetic -- they get added,
     * subtracted, split between winners and compared against a stack --
     * so wrapping them in an opaque handle would cost more than the
     * type safety is worth here.
     */
    using Chips = std::uint64_t;

} // namespace antwika::holdem
