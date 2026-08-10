#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
#include <antwika/cli/CommandLine.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/mapcheck_cli/CheckMaps.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::runGuarded;
using antwika::log::Level;
using antwika::mapcheck_cli::checkMaps;

namespace
{
    constexpr std::string_view kName = "antwika_mapcheck_cli";
}

int main(int argc, char **argv)
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    bool clean = true;

    const int guarded = runGuarded(
        kName,
        [&logger, &clean, argc, argv]
        {
            std::vector<std::filesystem::path> paths{};
            for (int i = 1; i < argc; ++i)
            {
                if (std::string_view(argv[i]) == antwika::cli::kHelpFlag)
                {
                    std::cout << antwika::cli::helpText(kName, {});
                    return;
                }
                paths.emplace_back(argv[i]);
            }

            if (paths.empty())
            {
                std::cout << "no maps given\n";
                return;
            }

            logger.log(
                Level::Info,
                "Checking " + std::to_string(paths.size()) + " map(s)");

            clean = checkMaps(paths, std::cout);
        });

    if (guarded != EXIT_SUCCESS)
    {
        return guarded;
    }

    return clean ? EXIT_SUCCESS : EXIT_FAILURE;
}
