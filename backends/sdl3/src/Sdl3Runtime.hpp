#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <stdexcept>
#include <string_view>

#include <antwika/log/ILogger.hpp>

namespace antwika::sdl3
{

    using antwika::log::ILogger;

    class Sdl3Error final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class Sdl3Runtime final
    {
    public:
        [[nodiscard]] static std::shared_ptr<Sdl3Runtime> acquire(
            ILogger &logger);

        explicit Sdl3Runtime(ILogger &logger);

        Sdl3Runtime(const Sdl3Runtime &) = delete;
        Sdl3Runtime(Sdl3Runtime &&) = delete;

        Sdl3Runtime &operator=(const Sdl3Runtime &) = delete;
        Sdl3Runtime &operator=(Sdl3Runtime &&) = delete;

        ~Sdl3Runtime();
    };

    class Sdl3Subsystem final
    {
    public:
        Sdl3Subsystem(
            ILogger &logger, SDL_InitFlags flags, std::string_view name);

        Sdl3Subsystem(const Sdl3Subsystem &) = delete;
        Sdl3Subsystem(Sdl3Subsystem &&) = delete;

        Sdl3Subsystem &operator=(const Sdl3Subsystem &) = delete;
        Sdl3Subsystem &operator=(Sdl3Subsystem &&) = delete;

        ~Sdl3Subsystem();

    private:
        std::shared_ptr<Sdl3Runtime> runtime;
        SDL_InitFlags claimed;
    };

}
