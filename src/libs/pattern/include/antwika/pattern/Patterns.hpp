#pragma once

#include <vector>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/Pattern.hpp"

namespace antwika::pattern
{

    /**
     * @brief A pattern holding nothing at all.
     *
     * The identity of stack(), and what a combinator handed nothing
     * returns.
     *
     * @return The pattern, which answers every query with no events.
     */
    [[nodiscard]] Pattern silence();

    /**
     * @brief A pattern repeating one value, once per cycle.
     *
     * The atom everything else is built from.
     * Queried across three cycles it gives three events, each whole
     * covering its own cycle.
     *
     * @param value What every event carries.
     * @return The pattern.
     */
    [[nodiscard]] Pattern pure(Controls value);

    /**
     * @brief A value that is simply always there.
     *
     * **Continuous rather than an event**, which is the distinction Hap
     * draws with its optional whole: this never *begins*, so it never
     * has an onset and a sequencer never triggers on it.
     * It is read as a parameter instead, which is what automating a
     * filter cutoff or a gain across a run is made of.
     *
     * @param value What is always there.
     * @return The pattern, answering any window with one whole-less
     * event covering it.
     */
    [[nodiscard]] Pattern steady(Controls value);

    /**
     * @brief Sound several patterns at once.
     * @param layers What to play together.
     * @return The pattern, whose events are every layer's events.
     */
    [[nodiscard]] Pattern stack(std::vector<Pattern> layers);

    /**
     * @brief Give each pattern one cycle in turn.
     *
     * Strudel calls this `cat` or `slowcat`: the first pattern plays in
     * cycle zero, the second in cycle one, and so round again.
     *
     * @param parts What to play in turn.
     * @return The pattern.
     * @throws PatternError If nothing was given.
     */
    [[nodiscard]] Pattern slowcat(std::vector<Pattern> parts);

    /**
     * @brief Fit every pattern into one cycle, side by side.
     *
     * Strudel calls this `fastcat` or `seq`, and it is what a bare
     * sequence in the mini-notation means.
     *
     * @param parts What to play, in order, sharing one cycle.
     * @return The pattern.
     * @throws PatternError If nothing was given.
     */
    [[nodiscard]] Pattern fastcat(std::vector<Pattern> parts);

    /**
     * @brief One slice of a weighted sequence.
     */
    struct Slice
    {
        /** @brief How much of the cycle this slice holds, over the
         * sum of every slice's weight. */
        Cycle weight{1};

        /** @brief What plays inside it. */
        Pattern part = silence();
    };

    /**
     * @brief Fit every pattern into one cycle, each as wide as its
     * weight says.
     *
     * Tidal calls this `timeCat`, and it is what the mini-notation's
     * tie is read into: `"0 _ 3"` is a slice of weight two and one of
     * weight one.
     * With every weight equal it plays exactly as fastcat does; the
     * weights are what fastcat cannot say.
     *
     * One cycle of a slice's pattern is squeezed into each of that
     * slice's occurrences, and the cycle asked for advances with the
     * outer one -- so an alternation inside a slice still turns once
     * per cycle, exactly as it would in a fastcat.
     *
     * @param parts What to play, in order, sharing one cycle by
     * weight.
     * @return The pattern.
     * @throws PatternError If nothing was given, or any weight is
     * nothing or less.
     */
    [[nodiscard]] Pattern timecat(std::vector<Slice> parts);

} // namespace antwika::pattern
