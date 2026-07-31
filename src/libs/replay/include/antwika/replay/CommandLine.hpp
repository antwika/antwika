#pragma once

#include <antwika/cli/CommandLine.hpp>

namespace antwika::replay
{

    // Parsing a command line is antwika::cli's now.
    // That library depends on nothing at all.
    // Naming a flag never needed a replay format linked to read it.
    // What is left here re-exports the names under the old spelling.
    // So a caller still writing antwika::replay:: keeps compiling.
    // There is no second implementation to drift from the first.
    // These go once every caller names antwika::cli itself.
    using antwika::cli::CommandLine;
    using antwika::cli::helpText;
    using antwika::cli::kHelpFlag;
    using antwika::cli::parseCommandLine;

} // namespace antwika::replay
