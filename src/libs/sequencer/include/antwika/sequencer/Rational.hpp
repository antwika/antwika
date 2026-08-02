#pragma once

#include <antwika/pattern/Cycle.hpp>

namespace antwika::sequencer
{

    /**
     * @brief An exact fraction, used here for rates and durations.
     *
     * The same type antwika::pattern measures musical position with,
     * under a second name because this library also uses it for things
     * that are not positions -- frames per tick, frames per cycle.
     *
     * A second *type* would be a second implementation of exact rational
     * arithmetic, with its own overflow rules to get wrong.
     * The alias says the reuse is deliberate rather than accidental.
     */
    using Rational = antwika::pattern::Cycle;

} // namespace antwika::sequencer
