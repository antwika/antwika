#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/time/SystemSleeper.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
#include <antwika/cli/CommandLine.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/gfx3d_demo/CubeMesh.hpp"
#include "antwika/gfx3d_demo/SpinLoop.hpp"
#include "antwika/gfx3d_demo/SpinScene.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::runGuarded;
using antwika::gfx::WindowDesc;
using antwika::gfx3d_demo::cubeMesh;
using antwika::gfx3d_demo::SpinLoop;
using antwika::gfx3d_demo::SpinScene;
using antwika::log::Level;

namespace
{
    constexpr std::chrono::milliseconds kFramePeriod{16};

    constexpr std::string_view kName = "antwika_gfx3d_demo";

    constexpr std::optional<std::uint32_t> kDemoFrames{900};
}

int main(int argc, char **argv)
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    return runGuarded(
        kName,
        [&logger, argc, argv]
        {
            const auto command =
                antwika::cli::parseCommandLine(argc, argv, {});

            if (command.has(antwika::cli::kHelpFlag))
            {
                std::cout << antwika::cli::helpText(kName, {});
                return;
            }

            const auto backend = antwika::gfx::makeSelectedBackend(logger);

            logger.log(
                Level::Info,
                "Antwika gfx3d demo on backend: "
                    + std::string(backend->name()));

            const SpinScene scene;
            antwika::time::SystemSleeper sleeper;
            SpinLoop loop(*backend, scene, sleeper, kFramePeriod);

            loop.run(
                WindowDesc{
                    .title = "Antwika gfx3d demo",
                    .size = {.width = 800, .height = 600}},
                cubeMesh(),
                kDemoFrames);

            logger.log(
                Level::Info,
                "Antwika gfx3d demo drew "
                    + std::to_string(loop.ticks()) + " ticks");
        });
}
