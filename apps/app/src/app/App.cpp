#include "App.hpp"

#include <iostream>
#include <antwika/log/Logger.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/engine/Engine.hpp>

namespace antwika::app
{

    void App::run() {
        antwika::log::StreamAppender appender(std::cout);
        antwika::log::Logger logger(antwika::log::Level::Info, appender);

        antwika::engine::Engine engine(logger);
        engine.start();
    }

} // namespace antwika::app
