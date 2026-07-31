#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <stdexcept>
#include <string_view>

#include <antwika/log/ILogger.hpp>

namespace antwika::sdl3
{

    using antwika::log::ILogger;

    /**
     * @brief Thrown when SDL itself failed to start.
     *
     * Private to this directory, and never seen above it: Sdl3Backend
     * turns it into a GfxError, Sdl3InputBackend into an InputError and
     * Sdl3SoundBackend into a SoundError, so no library above here
     * learns that SDL is what it was built against.
     */
    class Sdl3Error final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief SDL itself, started once and shut down once.
     *
     * SDL_Init and SDL_Quit are process-global, and this directory now
     * has more than one seam that needs SDL up: the window and input
     * pump, and the sound backend.  Whichever arrives first starts the
     * library and whichever leaves last shuts it down.
     *
     * It initialises **no subsystem at all**.  Subsystems are claimed
     * separately, by Sdl3Subsystem, because a build selecting sdl3 for
     * graphics only must not find itself opening an audio device, and
     * one selecting it for sound only must not need a display.
     *
     * Reference-counted through a weak_ptr in a function-local static,
     * following the same idiom Sdl3Pump uses.  Not thread-safe, which
     * costs nothing: SDL wants its subsystems handled from one thread.
     */
    class Sdl3Runtime final
    {
    public:
        /**
         * @brief Get the process's runtime, starting SDL if it is down.
         * @param logger Receives the runtime's diagnostics.
         * @return The shared runtime, never null.
         * @throws Sdl3Error If SDL itself failed to start.
         */
        [[nodiscard]] static std::shared_ptr<Sdl3Runtime> acquire(
            ILogger &logger);

        /**
         * @brief Start SDL with no subsystems.
         *
         * Public only because make_shared needs it; go through
         * acquire().
         *
         * @param logger Receives the runtime's diagnostics.
         * @throws Sdl3Error If SDL itself failed to start.
         */
        explicit Sdl3Runtime(ILogger &logger);

        Sdl3Runtime(const Sdl3Runtime &) = delete;
        Sdl3Runtime(Sdl3Runtime &&) = delete;

        Sdl3Runtime &operator=(const Sdl3Runtime &) = delete;
        Sdl3Runtime &operator=(Sdl3Runtime &&) = delete;

        /**
         * @brief Shut SDL down.
         */
        ~Sdl3Runtime();
    };

    /**
     * @brief One SDL subsystem, held for as long as this lives.
     *
     * SDL reference-counts subsystems itself, which is exactly what
     * SDL_InitSubSystem exists for, so two holders of the same subsystem
     * are ordinary rather than a conflict.
     *
     * It keeps the runtime alive alongside it, so SDL_Quit can never run
     * before the last SDL_QuitSubSystem -- an ordering that would
     * otherwise have to be remembered separately by every holder.
     */
    class Sdl3Subsystem final
    {
    public:
        /**
         * @brief Claim a subsystem, starting SDL if it is down.
         * @param logger Receives the diagnostics.
         * @param flags Which subsystem, as an SDL_INIT_* flag.
         * @param name What to call it in a message.
         * @throws Sdl3Error If SDL or the subsystem failed to start.
         */
        Sdl3Subsystem(
            ILogger &logger, SDL_InitFlags flags, std::string_view name);

        Sdl3Subsystem(const Sdl3Subsystem &) = delete;
        Sdl3Subsystem(Sdl3Subsystem &&) = delete;

        Sdl3Subsystem &operator=(const Sdl3Subsystem &) = delete;
        Sdl3Subsystem &operator=(Sdl3Subsystem &&) = delete;

        /**
         * @brief Release the subsystem.
         */
        ~Sdl3Subsystem();

    private:
        std::shared_ptr<Sdl3Runtime> runtime;
        SDL_InitFlags claimed;
    };

} // namespace antwika::sdl3
