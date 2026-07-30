#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::input
{

    /**
     * @brief A button on a pointing device.
     *
     * Named by position rather than by handedness: Left is the button
     * under the index finger of a right-handed mouse, which the window
     * system has already swapped for a user who asked it to.
     *
     * X1 and X2 are the two side buttons a mouse may carry, named after
     * what the window systems call them rather than after the "back" and
     * "forward" a browser happens to bind them to.
     */
    enum class MouseButton : std::uint8_t
    {
        Left = 0,
        Middle,
        Right,
        X1,
        X2,
    };

    /**
     * @brief How many buttons this vocabulary holds.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kMouseButtonCount =
        static_cast<std::size_t>(MouseButton::X2) + 1;

    /**
     * @brief Get a button's index, for addressing a per-button array.
     * @param button The button to index.
     * @return The index, always below kMouseButtonCount for a named
     * button.
     */
    [[nodiscard]] constexpr std::size_t mouseButtonIndex(
        MouseButton button) noexcept
    {
        return static_cast<std::size_t>(button);
    }

    /**
     * @brief Get a button's stable, persisted name.
     *
     * This is what a replay stores, so these names are part of the replay
     * format and may not be changed once written.
     *
     * A value with no name is refused rather than named "Unknown", for
     * the reason toString(Key) gives: mouseButtonFromString rejects a
     * name no button goes by, so the lenient answer would write a
     * recording that encodes cleanly and then fails to replay.
     *
     * @param button The button to name.
     * @return The button's name, e.g. "Left".
     * @throws InputError If the value is outside the enumeration, which
     * only a cast can produce.
     */
    [[nodiscard]] std::string_view toString(MouseButton button);

    /**
     * @brief Get the button a persisted name refers to.
     * @param name The name to look up, as produced by toString().
     * @return The button that name refers to.
     * @throws InputError If no button goes by that name.
     */
    [[nodiscard]] MouseButton mouseButtonFromString(std::string_view name);

} // namespace antwika::input
