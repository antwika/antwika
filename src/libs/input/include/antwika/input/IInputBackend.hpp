#pragma once

#include <optional>
#include <string_view>

#include "antwika/input/InputCapabilities.hpp"
#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    /**
     * @brief Reports what a keyboard and a pointer did.
     *
     * The one seam between Antwika and a concrete input framework.
     * Exactly one implementation is compiled into a given build, chosen by
     * ANTWIKA_INPUT_BACKEND, so no code above this interface names SDL,
     * raylib or anything like them.
     *
     * Deliberately says nothing about windows. Reading input does not
     * require opening one, and this library does not depend on
     * antwika::gfx, so an event does not report which surface it arrived
     * at. That defers multi-window input routing, which would need a
     * second id vocabulary alongside gfx::WindowId and is worth designing
     * against a real two-window application rather than guessing at.
     */
    class IInputBackend
    {
    public:
        virtual ~IInputBackend() = default;

        /**
         * @brief Get the backend's name, for logs and diagnostics.
         * @return A stable identifier, e.g. "null".
         */
        [[nodiscard]] virtual std::string_view name() const = 0;

        /**
         * @brief Get which devices this backend deals in.
         *
         * Not every input source has both, and a backend that has only one
         * says so here rather than reporting events for a device it does
         * not have.
         *
         * @return The devices this backend reports. Never all false.
         */
        [[nodiscard]] virtual InputCapabilities capabilities() const = 0;

        /**
         * @brief Take the next event reported since the last call.
         *
         * Never blocks: an empty queue is reported, not waited on. A
         * backend reading pollable state rather than a queue must latch
         * what it has already reported, so that a caller draining events
         * between ticks reaches the end of them.
         *
         * @return The next event, or nullopt when none is pending.
         */
        [[nodiscard]] virtual std::optional<InputEvent> pollEvent() = 0;
    };

} // namespace antwika::input
