#include "antwika/app/RunCatchingErrors.hpp"

#include <cstdlib>
#include <exception>

namespace antwika::app
{

    int runCatchingErrors(
        std::string_view name,
        const std::function<void()> &body,
        std::ostream &errors)
    {
        try
        {
            body();
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            errors << name << ": " << error.what() << '\n';
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

}
