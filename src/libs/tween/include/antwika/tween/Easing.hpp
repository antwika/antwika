#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::tween
{

    /**
     * @brief Which curve a fraction is shaped by.
     *
     * Every one of these is a polynomial or a piecewise polynomial, which
     * is the whole of why this list is shorter than the one a tweening
     * library usually ships.
     * A `Progress` is an exact rational and this library keeps it exact,
     * so an easing is only expressible here if it can be computed with
     * integer arithmetic alone.
     *
     * **Sine, exponential, circular and elastic are deliberately absent.**
     * Each needs a transcendental or a square root, and none of those
     * agrees to the last bit across GNU, LLVM and MinGW -- so an eased
     * position would be a pixel out on one toolchain and not another, and
     * the drawn-position assertions this project relies on would have to
     * become assertions with a tolerance.
     *
     * **Back and anticipate are absent for a second, separate reason.**
     * They overshoot: back-in dips below zero before it climbs, and
     * back-out passes one before it settles.
     * A `Progress` is between zero and one inclusive by construction, so
     * an overshoot is not merely inexact here, it is unrepresentable.
     * Admitting one would mean widening `Progress`, which is a change to
     * antwika::animation and a decision of its own.
     *
     * Values are contiguous from zero, so an easing can index a table.
     */
    enum class Easing : std::uint8_t
    {
        Linear = 0,

        QuadIn,
        QuadOut,
        QuadInOut,

        CubicIn,
        CubicOut,
        CubicInOut,

        QuartIn,
        QuartOut,
        QuartInOut,

        QuintIn,
        QuintOut,
        QuintInOut,

        BounceIn,
        BounceOut,
        BounceInOut,
    };

    /**
     * @brief How many easings there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kEasingCount =
        static_cast<std::size_t>(Easing::BounceInOut) + 1;

    /**
     * @brief Get an easing's index, for addressing a per-easing table.
     * @param easing The easing to index.
     * @return The index, always below kEasingCount for a named easing.
     */
    [[nodiscard]] constexpr std::size_t easingIndex(Easing easing) noexcept
    {
        return static_cast<std::size_t>(easing);
    }

    /**
     * @brief Get an easing's name, for a message a person reads.
     *
     * Symbolic rather than the enumerator's number, so a name survives
     * the enumeration being reordered.
     * Nothing persists one: this library holds no format, and an easing
     * is a build-time decision rather than something a save file carries.
     *
     * @param easing The easing to name.
     * @return Its name, or "unknown" for a value no enumerator has.
     */
    [[nodiscard]] std::string_view easingName(Easing easing) noexcept;

} // namespace antwika::tween
