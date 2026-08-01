#pragma once

#include <stdexcept>

namespace antwika::tower_defence
{

    /**
     * @brief Thrown when a high-score file is not one this build can
     * read, or cannot be written once it has been opened.
     *
     * That covers text that is not JSON, a document that is not an
     * object, a "magic" that is not this format's, a version this build
     * does not know how to reach, and a missing or misshapen member.
     *
     * Its own type rather than LevelError, following the
     * one-exception-type-per-failure-category rule: a level that cannot
     * be generated is a mistake in this build, where a file that will
     * not read is a mistake somewhere else entirely.
     *
     * **A file that is not there is deliberately not one of these.**
     * A first run has no best score, which is an ordinary answer worth
     * zero rather than a failure -- see IScoreStore::load().
     */
    class ScoreFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::tower_defence
