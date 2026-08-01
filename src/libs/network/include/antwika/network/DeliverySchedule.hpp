#pragma once

#include <cstddef>
#include <vector>

namespace antwika::network
{

    /**
     * @brief What a loopback network does to the packets crossing it.
     *
     * **Latency and loss are scripted rather than sampled**, which is
     * the whole point: a test that drops the third packet drops the
     * third packet on every machine and every toolchain, where one
     * drawing from a generator would be a different test every run and
     * a different one again under instrumentation.
     *
     * It is a plain value with no clock and no generator behind it, so
     * a two-peer session under an adversarial network costs no
     * wall-clock time at all -- see LoopbackBackend.
     */
    struct DeliverySchedule
    {
        /**
         * @brief How many pumps a packet waits before it can be
         * received.
         *
         * Counted on the *receiving* host, so zero means the next pump
         * after the send hands it over, and two means the third does.
         */
        std::size_t delayPumps = 0;

        /**
         * @brief Which sends to throw away, by their ordinal.
         *
         * Counted from zero across the whole network rather than per
         * host, since a script naming "the third packet" means the
         * third one anybody sent.
         * A broadcast spends one ordinal per peer it reaches, because
         * that is how many packets it is.
         */
        std::vector<std::size_t> dropped;

        /**
         * @brief Compare two schedules.
         * @param other The schedule to compare against.
         * @return True when the delay and every dropped ordinal match.
         */
        [[nodiscard]] bool operator==(const DeliverySchedule &other) const
            = default;
    };

} // namespace antwika::network
