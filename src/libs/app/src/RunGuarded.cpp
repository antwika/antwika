#include "antwika/app/RunGuarded.hpp"

#include <cstdlib>
#include <exception>

namespace antwika::app
{

    int runGuarded(
        std::string_view name,
        const std::function<void()> &body,
        std::ostream &errors)
    {
        try
        {
            body();
        }
        // gcov -b tags this handler's non-matching edge, at 0.
        // There is one handler here.
        // The unwinder enters a frame only when it holds a match.
        // So the compare it lands on can only go the one way.
        // A throw that does not match unwinds straight past.
        // That is what RunGuardedTest asserts about an int.
        // Every catch in src/ carries this marker for that reason.
        // See docs/confirming-unreachable-branches.md.
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            errors << name << ": " << error.what() << '\n';
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

} // namespace antwika::app
