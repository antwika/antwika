#include <cstdint>
#include <iostream>
#include <string>

#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/gfx_demo/DemoLoop.hpp"
#include "antwika/gfx_demo/DemoScene.hpp"

using antwika::gfx::WindowDesc;
using antwika::gfx_demo::DemoLoop;
using antwika::gfx_demo::DemoScene;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::time::SystemClock;

namespace
{
    constexpr std::uint32_t kDemoFrames = 3;
} // namespace

int main()
{
    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    Logger logger(formatter, logPolicy, clock, appender);

    const auto backend = antwika::gfx::makeSelectedBackend(logger);

    logger.log(
        Level::Info,
        "Antwika gfx demo on backend: " + std::string(backend->name()));

    const DemoScene scene;
    DemoLoop loop(*backend, scene);

    loop.run(
        WindowDesc{
            .title = "Antwika gfx demo",
            .size = {.width = 800, .height = 600}},
        kDemoFrames);

    return 0;
}
