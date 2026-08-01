#pragma once

#include <stdexcept>

namespace antwika::companion
{

    /**
     * @brief Thrown when a companion file is not one this build can
     * read, or cannot be written once it has been opened.
     *
     * That covers text that is not JSON, a document that is not an
     * object, a "magic" that is not this format's, a version this build
     * does not know how to reach, a missing or misshapen member, and a
     * set of numbers no live companion could be in.
     *
     * Its own type rather than CompanionError, following the
     * one-exception-type-per-failure-category rule: a companion balanced
     * on numbers no run could use is a mistake in this build, where a
     * file that will not read is a mistake somewhere else entirely, and
     * a caller offering to carry on without one wants to catch only the
     * second.
     *
     * A file that is not there is deliberately *not* one of these: a
     * first run has no previous companion, which is an ordinary answer
     * rather than a failure -- see IPetStore::load().
     */
    class SaveFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::companion
