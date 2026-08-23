#include <array>
#include <iostream>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunCatchingErrors.hpp>
#include <antwika/cli/CommandLine.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/input/SelectedInputBackend.hpp>

#include "antwika/game/app/Runner.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::runCatchingErrors;
using antwika::game::kAppName;
using antwika::game::Runner;
using antwika::log::Level;

namespace
{
    constexpr std::array kFlags = {
        antwika::cli::FlagSpec{
            .name = "--map",
            .valueName = "path",
            .help = "The map to play, holding the voxels, the "
                    "tilemap and the roster. Defaults to "
                    "assets/maps/map.json."},
    };

    constexpr std::string_view kDefaultMap = "assets/maps/map.json";
}

int main(int argc, char **argv)
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    return runCatchingErrors(
        kAppName,
        [&logger, argc, argv]
        {
            const auto command =
                antwika::cli::parseCommandLine(argc, argv, kFlags);

            if (command.has(antwika::cli::kHelpFlag))
            {
                std::cout << antwika::cli::helpText(kAppName, kFlags);

                return;
            }

            const auto backend = antwika::gfx::makeSelectedBackend(logger);
            const auto inputs =
                antwika::input::makeSelectedInputBackend(logger);

            Runner(
                logger,
                *backend,
                *inputs,
                std::string(
                    command.value("--map").value_or(
                        std::string(kDefaultMap))))
                .run();
        });
}
