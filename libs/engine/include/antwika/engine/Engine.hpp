#pragma once

#include <antwika/log/Logger.hpp>

#ifdef _WIN32
#define ENGINE_EXPORT __declspec(dllexport)
#else
#define ENGINE_EXPORT
#endif

namespace antwika::engine
{

    class ENGINE_EXPORT Engine
    {
    public:
        Engine(antwika::log::ILogger &logger);

        void start();

    private:
        antwika::log::ILogger &logger;
    };

} // namespace antwika::engine
