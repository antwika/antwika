#pragma once

#include <cstdint>
#include <vector>

#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/ParamValue.hpp"
#include "antwika/pattern/Pattern.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    /**
     * @brief Play a pattern faster, fitting more cycles into one.
     * @param factor How much faster; two plays it twice per cycle.
     * @param inner What to speed up.
     * @return The pattern.
     * @throws PatternError If the factor is zero or negative.
     */
    [[nodiscard]] Pattern fast(Cycle factor, Pattern inner);

    /**
     * @brief Play a pattern slower, spreading one cycle over more.
     * @param factor How much slower; two takes two cycles per cycle.
     * @param inner What to slow down.
     * @return The pattern.
     * @throws PatternError If the factor is zero or negative.
     */
    [[nodiscard]] Pattern slow(Cycle factor, Pattern inner);

    /**
     * @brief Shift a pattern earlier in time.
     * @param amount How far to bring it forward.
     * @param inner What to shift.
     * @return The pattern.
     */
    [[nodiscard]] Pattern early(Cycle amount, Pattern inner);

    /**
     * @brief Shift a pattern later in time.
     * @param amount How far to push it back.
     * @param inner What to shift.
     * @return The pattern.
     */
    [[nodiscard]] Pattern late(Cycle amount, Pattern inner);

    /**
     * @brief Play each cycle of a pattern backwards.
     *
     * Reflected inside the cycle it falls in rather than around zero,
     * so a pattern reversed twice is the pattern again and a reversal
     * never moves an event into another cycle.
     *
     * @param inner What to reverse.
     * @return The pattern.
     */
    [[nodiscard]] Pattern rev(Pattern inner);

    /**
     * @brief Keep a pattern's events only on a Euclidean rhythm.
     *
     * Bjorklund's algorithm: `pulses` onsets spread as evenly as whole
     * steps allow across `steps` slots, which is where nearly every
     * traditional rhythm in the world comes from.
     *
     * @param pulses How many slots sound.
     * @param steps How many slots there are.
     * @param inner What to sound on them.
     * @return The pattern, one cycle long.
     * @throws PatternError If there are no steps, if either count is
     * negative, or if more pulses were asked for than there are steps.
     */
    [[nodiscard]] Pattern euclid(
        std::int64_t pulses, std::int64_t steps, Pattern inner);

    /**
     * @brief Drop events at random, reproducibly.
     *
     * **Positional rather than generated**, which is what lets a
     * sequencer ask about cycle four hundred directly and get the same
     * answer as one that played its way there.
     * A generator advanced per event would make the answer depend on
     * how many events had been asked for and in what order, which
     * breaks both that property and replay the moment a lookahead
     * window changes size.
     *
     * @param chance How likely an event is to be dropped, from zero to
     * one.
     * @param seed What makes one degradation differ from another.
     * @param inner What to thin out.
     * @return The pattern.
     * @throws PatternError If the chance lies outside zero to one.
     */
    [[nodiscard]] Pattern degradeBy(
        ParamValue chance, std::uint64_t seed, Pattern inner);

    /**
     * @brief Play a pattern inside scheduled windows of a period.
     *
     * **Each window replays the pattern from its own start**: the
     * window opening at cycle twelve hands the pattern its cycle
     * zero, so every occurrence of a section sounds the same,
     * wherever the schedule puts it.
     * That is deliberately not slowcat's arithmetic, which hands a
     * slot its pattern's *nth* cycle and so advances an alternation
     * once per revolution rather than once per cycle.
     *
     * Outside every window is silence, and the whole schedule
     * repeats every `period` cycles.
     * An event cut by its window's edge keeps its whole, so what
     * began inside may ring past the boundary; nothing begins
     * outside.
     *
     * @param period How many cycles the schedule spans before it
     * repeats.
     * @param windows Where inside one period the pattern plays, in
     * order.
     * @param inner What to play there.
     * @return The pattern.
     * @throws PatternError If the period is below one, the schedule
     * holds no windows, or the windows overlap, run out of order or
     * leave the period.
     */
    [[nodiscard]] Pattern during(
        std::int64_t period, std::vector<Span> windows, Pattern inner);

} // namespace antwika::pattern
