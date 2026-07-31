#pragma once

#include <functional>
#include <iostream>
#include <ostream>
#include <string_view>

namespace antwika::app
{

    /**
     * @brief Run a session, report what it threw, and turn that into an
     * exit code.
     *
     * This is the half of runRecorded() that has nothing to do with
     * replays, and it exists so that an app's main.cpp can be a single
     * call with no try of its own.
     * The style guide asks for exactly that, and two demos had a
     * line-for-line copy of this instead.
     *
     * Catching is what makes the run's resources unwind at all: an
     * uncaught exception may call std::terminate without unwinding, and
     * a window, a texture or a mesh is what would be left behind.
     *
     * A throw that is not a std::exception is deliberately let through.
     * It is a bug in the body rather than a failed run, and reporting it
     * as a failed one would name a message it does not have.
     *
     * @param name The program's name, used to prefix a failure.
     * @param body The session to run.
     * @param errors Where a failure is reported.
     * @return EXIT_SUCCESS, or EXIT_FAILURE if the body threw a
     * std::exception.
     */
    int runGuarded(
        std::string_view name,
        const std::function<void()> &body,
        std::ostream &errors = std::cerr);

} // namespace antwika::app
