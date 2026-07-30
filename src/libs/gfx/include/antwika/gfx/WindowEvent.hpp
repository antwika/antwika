#pragma once

#include <variant>

#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    /**
     * @brief The window's close control was activated.
     *
     * A request, not a fact: the window stays open until something calls
     * IWindow::close(), so an application can ignore it or ask first.
     */
    struct CloseRequested
    {
        /**
         * @brief Compare two close requests.
         * @param other The request to compare against.
         * @return Always true: the type carries no state.
         */
        [[nodiscard]] bool operator==(
            const CloseRequested &other) const = default;
    };

    /**
     * @brief The window's drawable area changed size.
     */
    struct Resized
    {
        Size size;

        /**
         * @brief Compare two resizes.
         * @param other The resize to compare against.
         * @return True when both report the same size.
         */
        [[nodiscard]] bool operator==(const Resized &other) const = default;
    };

    /**
     * @brief What happened, without saying who it happened to.
     *
     * Deliberately limited to lifetime events. Keyboard and pointer input
     * belong with the work that feeds live input into replays, which is
     * out of scope until there is a live input source to record from.
     */
    using WindowEventPayload = std::variant<CloseRequested, Resized>;

    /**
     * @brief Something a window reported, and which window reported it.
     *
     * The id is not decoration. A backend pumps one queue for every
     * window it owns -- that is how the underlying frameworks work -- so
     * an application with two windows open cannot act on a close request
     * without being told which window it refers to.
     */
    struct WindowEvent
    {
        WindowId window = kNullWindowId;
        WindowEventPayload payload;

        /**
         * @brief Compare two events.
         * @param other The event to compare against.
         * @return True when both the window and the payload match.
         */
        [[nodiscard]] bool operator==(const WindowEvent &other) const = default;
    };

} // namespace antwika::gfx
