#pragma once

#include <SDL3/SDL.h>

#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <stdexcept>

#include <antwika/log/ILogger.hpp>

namespace antwika::sdl3
{

    using antwika::log::ILogger;

    /**
     * @brief Thrown when SDL itself failed to start.
     *
     * Private to this directory, and never seen above it: Sdl3Backend
     * turns it into a GfxError and Sdl3InputBackend into an InputError, so
     * neither library learns that SDL is what it was built against.
     */
    class Sdl3PumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief The one place SDL's single event queue is admitted.
     *
     * SDL_PollEvent drains one process-global queue holding window,
     * keyboard and mouse events together. antwika::gfx and antwika::input
     * are deliberately independent of each other, so two backends over
     * this one framework would each poll it and silently eat the events
     * the other was waiting for, depending on which polled first.
     *
     * This is where that is resolved, rather than upward as a rule the two
     * libraries would have to cooperate on. One call to SDL_PollEvent
     * routes each event into a window queue or an input queue, so whoever
     * polls first advances both.
     *
     * Reference-counted through a weak_ptr in a function-local static, so
     * one process holds one pump however many backends ask for it, and
     * SDL_Init/SDL_Quit happen exactly once around all of them. Not
     * thread-safe, which costs nothing: SDL requires its event queue to be
     * pumped from the thread that initialised video anyway.
     *
     * It stores SDL's own events, untranslated. Translating here would
     * make the pump depend on both antwika::gfx and antwika::input, and a
     * build selecting SDL for only one of them would then link the other
     * for no reason.
     */
    class Sdl3Pump final
    {
    public:
        /**
         * @brief How many events one queue holds before dropping oldest.
         *
         * A cap is needed because an application polling one subsystem and
         * not the other would otherwise grow the unread queue without
         * limit. Dropping the oldest is the right trade for a queue nobody
         * is reading, and generous enough that a caller draining every
         * tick never reaches it.
         */
        static constexpr std::size_t kQueueLimit = 4096;

        /**
         * @brief Get the process's pump, starting SDL if it is not up.
         * @param logger Receives the pump's diagnostics.
         * @return The shared pump, never null.
         * @throws Sdl3PumpError If SDL's video subsystem failed to start.
         */
        [[nodiscard]] static std::shared_ptr<Sdl3Pump> acquire(
            ILogger &logger);

        /**
         * @brief Start SDL's video subsystem.
         *
         * Public only because make_shared needs it; go through acquire().
         *
         * @param logger Receives the pump's diagnostics.
         * @throws Sdl3PumpError If SDL's video subsystem failed to start.
         */
        explicit Sdl3Pump(ILogger &logger);

        Sdl3Pump(const Sdl3Pump &) = delete;
        Sdl3Pump(Sdl3Pump &&) = delete;

        Sdl3Pump &operator=(const Sdl3Pump &) = delete;
        Sdl3Pump &operator=(Sdl3Pump &&) = delete;

        /**
         * @brief Shut SDL's video subsystem down.
         */
        ~Sdl3Pump();

        /**
         * @brief Take the next window event SDL has reported.
         * @return The next event, or nullopt when none is pending.
         */
        [[nodiscard]] std::optional<SDL_Event> nextWindowEvent();

        /**
         * @brief Take the next keyboard or mouse event SDL has reported.
         * @return The next event, or nullopt when none is pending.
         */
        [[nodiscard]] std::optional<SDL_Event> nextInputEvent();

    private:
        [[nodiscard]] std::optional<SDL_Event> takeFrom(
            std::deque<SDL_Event> &queue);

        void drainSdl();

        static void enqueue(
            std::deque<SDL_Event> &queue, const SDL_Event &event);

        std::deque<SDL_Event> windowEvents;
        std::deque<SDL_Event> inputEvents;
    };

} // namespace antwika::sdl3
