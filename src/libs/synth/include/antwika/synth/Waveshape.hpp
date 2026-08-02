#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::synth
{

    /**
     * @brief Which shape one voice's oscillator traces.
     *
     * The four periodic shapes are the classic subtractive set, and they
     * are here because between them they cover what a game needs: a sine
     * for a tone, a saw for something bright, a square for something
     * hollow, and a triangle for something soft.
     *
     * **Noise is in this enumeration and is not periodic**, which is the
     * one thing about this list worth knowing.
     * It has no phase and no frequency, so it is a function of where a
     * voice has got to rather than of an angle -- see oscillate().
     *
     * Values are contiguous from zero, so a shape can index a table.
     */
    enum class Waveshape : std::uint8_t
    {
        Sine = 0,
        Saw,
        Square,
        Triangle,
        Noise,
    };

    /**
     * @brief How many shapes there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kWaveshapeCount =
        static_cast<std::size_t>(Waveshape::Noise) + 1;

    /**
     * @brief Get a shape's index, for addressing a per-shape table.
     * @param shape The shape to index.
     * @return The index, always below kWaveshapeCount for a named shape.
     */
    [[nodiscard]] constexpr std::size_t waveshapeIndex(
        Waveshape shape) noexcept
    {
        return static_cast<std::size_t>(shape);
    }

    /**
     * @brief Check whether a shape repeats over a phase.
     *
     * What tells a caller whether a frequency means anything for this
     * shape, which is what the trigger path validates on.
     *
     * @param shape The shape to ask about.
     * @return True for every shape but Noise, including a value no
     * enumerator has -- those are refused elsewhere rather than here.
     */
    [[nodiscard]] constexpr bool isPeriodic(Waveshape shape) noexcept
    {
        return shape != Waveshape::Noise;
    }

    /**
     * @brief Get a shape's name, for a message a person reads.
     *
     * Symbolic rather than the enumerator's number, so a name survives
     * the enumeration being reordered.
     * Nothing persists one: a shape is written into a description in
     * source and never into a file.
     *
     * @param shape The shape to name.
     * @return Its name, or "unknown" for a value no enumerator has.
     */
    [[nodiscard]] std::string_view waveshapeName(Waveshape shape) noexcept;

} // namespace antwika::synth
