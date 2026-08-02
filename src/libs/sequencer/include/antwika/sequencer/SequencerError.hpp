#pragma once

#include <stdexcept>

namespace antwika::sequencer
{

    /**
     * @brief Thrown when a run could not be laid out in time.
     *
     * Every cause is a description that names no timeline: a rate of no
     * frames a second, a tick interval of no time at all, a tempo of no
     * frames a cycle, a segment placed before one already added, or a
     * lookahead of no ticks.
     *
     * Arithmetic that will not fit exactly surfaces as
     * antwika::pattern::PatternError instead, because that is where the
     * exact rational lives and where the refusal belongs.
     *
     * Deliberately a single, specific, catchable type.
     */
    class SequencerError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::sequencer
