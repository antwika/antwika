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
        catch (const std::exception &error)
        {
            errors << name << ": " << error.what() << '\n';
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

} // namespace antwika::app
