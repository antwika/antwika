#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <deque>
#include <memory>
#include <optional>

#include <antwika/log/ILogger.hpp>

#include "Sdl3Runtime.hpp"

namespace antwika::sdl3
{

    using antwika::log::ILogger;

    class Sdl3Pump final
    {
    public:
        static constexpr std::size_t kQueueLimit = 4096;

        [[nodiscard]] static std::shared_ptr<Sdl3Pump> acquire(
            ILogger &logger);

        explicit Sdl3Pump(ILogger &logger);

        Sdl3Pump(const Sdl3Pump &) = delete;
        Sdl3Pump(Sdl3Pump &&) = delete;

        Sdl3Pump &operator=(const Sdl3Pump &) = delete;
        Sdl3Pump &operator=(Sdl3Pump &&) = delete;

        ~Sdl3Pump() = default;

        [[nodiscard]] std::optional<SDL_Event> nextWindowEvent();

        [[nodiscard]] std::optional<SDL_Event> nextInputEvent();

    private:
        [[nodiscard]] std::optional<SDL_Event> takeFrom(
            std::deque<SDL_Event> &queue);

        void drainSdl();

        static void enqueue(
            std::deque<SDL_Event> &queue, const SDL_Event &event);

        Sdl3Subsystem video;

        std::deque<SDL_Event> windowEvents;
        std::deque<SDL_Event> inputEvents;
    };

}
