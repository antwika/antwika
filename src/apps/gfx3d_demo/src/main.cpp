#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunGuarded.hpp>
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
    // Capped, rather than run until the window is closed.
    // That is where this parts company with gfx_demo.
    // The null backend reports no close at all.
    // It is also the default build every CI leg produces.
    // An uncapped run there would never finish.
    // A cap is what keeps a run reproducible, too.
    // The picture follows the tick count and nothing else.
    constexpr std::optional<std::uint32_t> kDemoFrames{900};
} // namespace

int main()
{
    ConsoleLogging logging(std::cout, Level::Info);
    auto &logger = logging.logger();

    return runGuarded(
        "antwika_gfx3d_demo",
        [&logger]
        {
            const auto backend = antwika::gfx::makeSelectedBackend(logger);

            logger.log(
                Level::Info,
                "Antwika gfx3d demo on backend: "
                    + std::string(backend->name()));

            const SpinScene scene;
            SpinLoop loop(*backend, scene);

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
