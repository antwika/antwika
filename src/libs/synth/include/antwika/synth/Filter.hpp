#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/sound/WaveFormat.hpp>

namespace antwika::synth
{

    using antwika::sound::SampleRate;

    /**
     * @brief Which part of a signal a filter keeps.
     *
     * None is a named mode rather than an absent filter, so a voice
     * always has one and no caller writes a branch to ask whether it
     * does.
     */
    enum class FilterMode : std::uint8_t
    {
        None = 0,
        LowPass,
        HighPass,
        BandPass,
    };

    /**
     * @brief How many modes there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kFilterModeCount =
        static_cast<std::size_t>(FilterMode::BandPass) + 1;

    /**
     * @brief What a filter is asked for, in a description a person wrote.
     *
     * In hertz rather than in a coefficient, because this is the half a
     * caller writes down; filterCoefficientsFor() is where it stops being
     * readable and starts being arithmetic.
     */
    struct FilterDesc
    {
        FilterMode mode = FilterMode::None;

        /** @brief Where it turns over, in hertz. */
        double cutoff = 0.0;

        /**
         * @brief How much it emphasises the cutoff.
         *
         * One is flat and smaller is sharper, so this is a damping figure
         * rather than a Q -- which is why a value of zero is refused
         * rather than meaning "none".
         */
        double resonance = 1.0;

        /**
         * @brief Compare two descriptions.
         * @param other The description to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const FilterDesc &other) const
            = default;
    };

    /**
     * @brief A filter's two coefficients, worked out once.
     *
     * Separate from FilterDesc because a cutoff in hertz is constant for
     * a voice's whole life, and turning it into a coefficient needs a
     * sine.
     * Doing that once when a voice is triggered rather than once per
     * sample is the difference between one transcendental per note and
     * forty-eight thousand.
     */
    struct FilterCoefficients
    {
        double frequency = 0.0;
        double damping = 0.0;

        /**
         * @brief Compare two sets of coefficients.
         * @param other The coefficients to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(const FilterCoefficients &other) const
            = default;
    };

    /**
     * @brief What a filter remembers between samples.
     *
     * The two integrator outputs of a state-variable filter, which is
     * the whole of its memory.
     * Projection state in the strictest sense: regenerable, in no
     * snapshot, and never read back into anything a replay reproduces.
     */
    struct FilterState
    {
        double low = 0.0;
        double band = 0.0;

        /**
         * @brief Compare two states.
         * @param other The state to compare against.
         * @return True when both integrators match.
         */
        [[nodiscard]] bool operator==(const FilterState &other) const
            = default;
    };

    /**
     * @brief Work out a description's coefficients at a rate.
     *
     * @param desc What the filter was asked for.
     * @param rate The rate the voice will run at.
     * @return The coefficients, clamped so the filter cannot run away
     * even if it was described with a cutoff near the rate.
     */
    [[nodiscard]] FilterCoefficients filterCoefficientsFor(
        const FilterDesc &desc, SampleRate rate) noexcept;

    /**
     * @brief Pass one sample through a state-variable filter.
     *
     * A Chamberlin state-variable filter, which gives all three modes
     * from one pair of integrators -- so there is one piece of arithmetic
     * to get right rather than three.
     *
     * @param mode Which output to take.
     * @param coefficients What filterCoefficientsFor() worked out.
     * @param state The filter's memory, advanced by this call.
     * @param input The sample to filter.
     * @return The filtered sample, or the input unchanged for None and
     * for a mode no enumerator names.
     */
    [[nodiscard]] float filterSample(
        FilterMode mode,
        const FilterCoefficients &coefficients,
        FilterState &state,
        float input) noexcept;

} // namespace antwika::synth
