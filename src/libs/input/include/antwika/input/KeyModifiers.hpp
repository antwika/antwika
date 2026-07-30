#pragma once

namespace antwika::input
{

    /**
     * @brief Which modifier keys were held when something happened.
     *
     * Four flags rather than a bitmask, because that is what makes a
     * modifier legible at the call site and in a replay, and because
     * nothing here needs to combine them arithmetically.
     *
     * Left and right are not distinguished: an application that cares
     * which shift key was pressed can bind Key::LeftShift directly.
     */
    struct KeyModifiers
    {
        bool shift = false;
        bool control = false;
        bool alt = false;
        bool super = false;

        /**
         * @brief Compare two sets of modifiers.
         * @param other The set to compare against.
         * @return True when the same modifiers were held in both.
         */
        [[nodiscard]] bool operator==(
            const KeyModifiers &other) const = default;
    };

} // namespace antwika::input
