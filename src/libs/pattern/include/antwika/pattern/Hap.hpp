#pragma once

#include <optional>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    /**
     * @brief One event, as a query saw it.
     *
     * **The two spans are the whole reason this type is not just a value
     * with a start and a length**, and getting them confused is the
     * single most expensive mistake available in this library.
     *
     * `part` is the stretch the query actually saw.
     * `whole` is the stretch the event would have covered had nothing
     * clipped it.
     * When a window cuts an event in half, two haps come back, each with
     * its own `part` and both carrying the same `whole`.
     *
     * A sequencer triggers on `hasOnset()` and never on a hap simply
     * existing.
     * Trigger on every hap and every sounding note restarts at every
     * window boundary, which is how a port of this idea comes out
     * sounding like a machine gun.
     *
     * A hap with no `whole` at all is a *continuous* value -- a signal
     * sampled over the window rather than an event that began.
     * It is read as a parameter and never triggered.
     */
    struct Hap
    {
        /** @brief What the event would have covered, if it began. */
        std::optional<Span> whole;

        /** @brief What the query saw. */
        Span part;

        /** @brief What the event carries. */
        Controls value;

        /**
         * @brief Get whether the event begins here.
         * @return True when it has a whole and this part starts at that
         * whole's start.
         */
        [[nodiscard]] bool hasOnset() const noexcept;

        /**
         * @brief Compare two events.
         * @param other The event to compare against.
         * @return True when both spans and the value all match.
         */
        [[nodiscard]] bool operator==(const Hap &other) const = default;
    };

} // namespace antwika::pattern
