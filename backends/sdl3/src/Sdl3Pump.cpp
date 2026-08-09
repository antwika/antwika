#include "Sdl3Pump.hpp"

#include <cstdint>

namespace antwika::sdl3
{

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
    }

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

    Sdl3Pump::Sdl3Pump(ILogger &logger)
        : video(logger, SDL_INIT_VIDEO, "video")
    {
    }

    std::optional<SDL_Event> Sdl3Pump::nextWindowEvent()
    {
        return takeFrom(windowEvents);
    }

    std::optional<SDL_Event> Sdl3Pump::nextInputEvent()
    {
        return takeFrom(inputEvents);
    }

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

}
