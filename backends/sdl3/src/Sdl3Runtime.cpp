#include "Sdl3Runtime.hpp"

#include <string>

#include <antwika/log/Level.hpp>

namespace antwika::sdl3
{

    using antwika::log::Level;

    std::shared_ptr<Sdl3Runtime> Sdl3Runtime::acquire(ILogger &logger)
    {
        static std::weak_ptr<Sdl3Runtime> shared;

        auto runtime = shared.lock();

        if (!runtime)
        {
            runtime = std::make_shared<Sdl3Runtime>(logger);
            shared = runtime;
        }

        return runtime;
    }

    Sdl3Runtime::Sdl3Runtime(ILogger &logger)
    {
        SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

        if (!SDL_Init(0))
        {
            throw Sdl3Error(
                std::string("sdl3: could not initialise SDL: ")
                + SDL_GetError());
        }

        logger.log(Level::Info, "sdl3: library started");
    }

    Sdl3Runtime::~Sdl3Runtime()
    {
        SDL_Quit();
    }

    Sdl3Subsystem::Sdl3Subsystem(
        ILogger &logger, SDL_InitFlags flags, std::string_view name)
        : runtime(Sdl3Runtime::acquire(logger)), claimed(flags)
    {
        if (!SDL_InitSubSystem(flags))
        {
            throw Sdl3Error(
                std::string("sdl3: could not initialise the ")
                + std::string(name) + " subsystem: " + SDL_GetError());
        }

        logger.log(
            Level::Info,
            "sdl3: " + std::string(name) + " subsystem started");
    }

    Sdl3Subsystem::~Sdl3Subsystem()
    {
        SDL_QuitSubSystem(claimed);
    }

}
