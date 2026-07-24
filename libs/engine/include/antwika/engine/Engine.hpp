#pragma once

#include <antwika/log/Logger.hpp>

namespace antwika::engine
{

    class Engine
    {
    public:
        Engine(antwika::log::ILogger &logger);

        void start();

    private:
        antwika::log::ILogger &logger;
    };

} // namespace antwika::engine
