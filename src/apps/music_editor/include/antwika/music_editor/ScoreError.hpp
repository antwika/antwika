#pragma once

#include <stdexcept>

namespace antwika::music_editor
{

    /**
     * @brief Thrown when a voice line could not be read.
     *
     * Every cause is this application's own syntax: a control no voice
     * has, an argument that is not a number, a chain with no `n(...)`
     * in it, a preset nothing is called.
     *
     * What the mini-notation *inside* an `n(...)` means is not this
     * error: a string that will not parse raises
     * antwika::notation::NotationError, and one that parses but asks
     * for something impossible raises antwika::pattern::PatternError.
     * The three arrive at the same place and read the same way beside
     * the line, but each is owned by whoever knows what went wrong.
     *
     * Deliberately a single, specific, catchable type.
     */
    class ScoreError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::music_editor
