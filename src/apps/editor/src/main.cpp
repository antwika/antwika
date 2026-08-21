#include <array>
#include <iostream>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunCatchingErrors.hpp>
#include <antwika/cli/CommandLine.hpp>

#include "antwika/editor/Editor.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::runCatchingErrors;
using antwika::editor::Editor;
using antwika::editor::kAppName;
using antwika::log::Level;

namespace
{
    constexpr std::array kFlags = {
        antwika::cli::FlagSpec{
            .name = "--map",
            .valueName = "path",
            .help = "The map ctrl+s saves to and f9 loads from, "
                    "holding the voxels and the tilemap. Defaults "
                    "to assets/maps/map.json."},
        antwika::cli::FlagSpec{
            .name = "--plan",
            .valueName = "path",
            .help = "The board of todo cards the plan view writes, "
                    "which stands apart from any one map. Defaults "
                    "to assets/plan.json."},
        antwika::cli::FlagSpec{
            .name = "--play",
            .valueName = "",
            .help = "Plays the map rather than opening the "
                    "editor: the game begins at once, escape "
                    "leaves it, and where the player has come to "
                    "is kept in progress.json beside the map."},
    };

    constexpr std::string_view kDefaultMap =
        "assets/maps/map.json";

    constexpr std::string_view kDefaultPlan = "assets/plan.json";
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
                std::cout << antwika::cli::helpText(
                    kAppName, kFlags);
                return;
            }

            Editor(
                logger,
                std::string(
                    command.value("--map").value_or(
                        std::string(kDefaultMap))),
                command.has("--play"),
                std::string(
                    command.value("--plan").value_or(
                        std::string(kDefaultPlan))))
                .run();
        });
}
