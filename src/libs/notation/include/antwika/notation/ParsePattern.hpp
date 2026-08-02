#pragma once

#include <string_view>

#include <antwika/pattern/Pattern.hpp>

#include "antwika/notation/IWordReader.hpp"

namespace antwika::notation
{

    using antwika::pattern::Pattern;

    /**
     * @brief Read a pattern string into a pattern.
     *
     * The mini-notation TidalCycles and Strudel are written in, over the
     * algebra antwika::pattern already provides.
     * **It is a translation and not a second implementation**: every
     * form below is spelled out in terms of a combinator that already
     * existed and was already tested, which is why this library needs
     * one error type and no semantics of its own.
     *
     * | Written | Means |
     * | --- | --- |
     * | `a b c` | one cycle split between them, in order |
     * | `~` | a rest |
     * | `[a b]` | a sequence occupying one slot |
     * | `[a, b]` | both at once |
     * | `<a b>` | one of them per cycle, in turn |
     * | `a*2` | twice as fast |
     * | `a/2` | half as fast |
     * | `a*3%2` | an exact ratio, never a decimal |
     * | `a!3` | three slots of it |
     * | `a(3,8)` | its onsets on a Euclidean rhythm |
     * | `a?` | half its events dropped, reproducibly |
     *
     * A speed is a whole number or a fraction of two written with `%`,
     * because an exact ratio is the point and a decimal would not be
     * one.
     *
     * Each `?` in one string gets its own seed, counted left to right,
     * so two of them thin out differently and both do so the same way
     * on every run.
     *
     * @param source The string to read.
     * @param words What one word of it means.
     * @return The pattern it describes.
     * @throws NotationError If the string does not parse.
     * @throws antwika::pattern::PatternError If it parses but describes
     * something no pattern could be -- nine onsets across eight steps,
     * a speed of nothing.
     */
    [[nodiscard]] Pattern parsePattern(
        std::string_view source, const IWordReader &words);

} // namespace antwika::notation
