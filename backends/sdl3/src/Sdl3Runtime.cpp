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

    // The logger is used here and not kept.
    // Nothing after construction has anything to say.
    Sdl3Runtime::Sdl3Runtime(ILogger &logger)
    {
        // SDL otherwise turns SIGINT and SIGTERM into an SDL_EVENT_QUIT.
        // No seam above here consumes that event.
        // Ctrl+C could then not stop a run with no window to close.
        // A run under SDL_VIDEODRIVER=dummy is exactly that.
        // Closing a window still arrives as a window event, unaffected.
        SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

        // No subsystem is asked for here, deliberately.
        // A build selecting sdl3 for one seam must not start another's.
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

} // namespace antwika::sdl3
