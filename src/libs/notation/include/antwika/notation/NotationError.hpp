#pragma once

#include <stdexcept>

namespace antwika::notation
{

    /**
     * @brief Thrown when a pattern string could not be read.
     *
     * Every cause is a *syntax* error: a bracket nothing closes, a
     * group holding nothing, a modifier with no number after it, a
     * character no rule mentions.
     *
     * A string that reads cleanly but asks for something impossible --
     * nine onsets across eight steps, a speed of zero -- raises
     * antwika::pattern::PatternError instead, from the combinator that
     * refused it.
     * The split is deliberate: this library owns the grammar, and the
     * algebra owns what the grammar means.
     *
     * Deliberately a single, specific, catchable type.
     */
    class NotationError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::notation
