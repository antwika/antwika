#pragma once

namespace antwika::input
{

    /**
     * @brief Which devices a backend deals in at all.
     *
     * Not every input source has both. A terminal backend has no pointer,
     * and saying so lets an application hide what it cannot offer instead
     * of waiting for events that will never arrive.
     *
     * At least one flag is always true: a backend that reports neither
     * device has nothing to report.
     */
    struct InputCapabilities
    {
        bool keyboard = false;
        bool pointer = false;

        /**
         * @brief Compare two sets of capabilities.
         * @param other The set to compare against.
         * @return True when both report the same devices.
         */
        [[nodiscard]] bool operator==(
            const InputCapabilities &other) const = default;
    };

} // namespace antwika::input
