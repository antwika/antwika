#include "Sdl3Pump.hpp"

#include <cstdint>
#include <string>

#include <antwika/log/Level.hpp>

namespace antwika::sdl3
{

    using antwika::log::Level;

    namespace
    {
        [[nodiscard]] bool isInputEvent(std::uint32_t type)
        {
            switch (type)
            {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            case SDL_EVENT_MOUSE_MOTION:
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            case SDL_EVENT_MOUSE_WHEEL:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool isWindowEvent(std::uint32_t type)
        {
            return type >= SDL_EVENT_WINDOW_FIRST
                   && type <= SDL_EVENT_WINDOW_LAST;
        }
    } // namespace

    std::shared_ptr<Sdl3Pump> Sdl3Pump::acquire(ILogger &logger)
    {
        static std::weak_ptr<Sdl3Pump> shared;

        auto pump = shared.lock();

        if (!pump)
        {
            pump = std::make_shared<Sdl3Pump>(logger);
            shared = pump;
        }

        return pump;
    }

    // The logger is used here and not kept.
    // Nothing after construction has anything to say.
    Sdl3Pump::Sdl3Pump(ILogger &logger)
    {
        // SDL otherwise turns SIGINT and SIGTERM into an SDL_EVENT_QUIT.
        // Neither seam above here consumes that event.
        // Ctrl+C could then not stop a run with no window to close.
        // A run under SDL_VIDEODRIVER=dummy is exactly that.
        // Closing a window still arrives as a window event, unaffected.
        SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            throw Sdl3PumpError(
                std::string("sdl3: could not initialise the video "
                            "subsystem: ")
                + SDL_GetError());
        }

        logger.log(Level::Info, "sdl3: video subsystem started");
    }

    Sdl3Pump::~Sdl3Pump()
    {
        SDL_Quit();
    }

    std::optional<SDL_Event> Sdl3Pump::nextWindowEvent()
    {
        return takeFrom(windowEvents);
    }

    std::optional<SDL_Event> Sdl3Pump::nextInputEvent()
    {
        return takeFrom(inputEvents);
    }

    // Whichever subsystem asks first is what advances both queues.
    std::optional<SDL_Event> Sdl3Pump::takeFrom(std::deque<SDL_Event> &queue)
    {
        if (queue.empty())
        {
            drainSdl();
        }

        if (queue.empty())
        {
            return std::nullopt;
        }

        const auto event = queue.front();
        queue.pop_front();

        return event;
    }

    void Sdl3Pump::drainSdl()
    {
        SDL_Event event;

        // What neither seam has a word for is dropped here.
        // That is what keeps the framework's own queue emptying.
        while (SDL_PollEvent(&event))
        {
            if (isWindowEvent(event.type))
            {
                enqueue(windowEvents, event);
            }
            else if (isInputEvent(event.type))
            {
                enqueue(inputEvents, event);
            }
        }
    }

    void Sdl3Pump::enqueue(
        std::deque<SDL_Event> &queue, const SDL_Event &event)
    {
        if (queue.size() == kQueueLimit)
        {
            queue.pop_front();
        }

        queue.push_back(event);
    }

} // namespace antwika::sdl3
