#pragma once

#include <antwika/cli/CommandLineError.hpp>

namespace antwika::replay
{

    // A refused command line is antwika::cli's error now.
    // This is a re-export rather than a second type; see CommandLine.hpp.
    // Catching either name catches the one thrown, since they are one.
    using antwika::cli::CommandLineError;

} // namespace antwika::replay
