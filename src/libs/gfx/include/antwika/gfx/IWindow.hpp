#pragma once

#include <string>
#include <string_view>

#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    /**
     * @brief One window, and the renderer that draws into it.
     *
     * A window is closed by calling close(), never by the backend
     * deciding on its own -- a CloseRequested event only asks.
     */
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        /**
         * @brief Get this window's id, for matching events against it.
         * @return An id distinct from every other live window of the same
         * backend, and never kNullWindowId.
         */
        [[nodiscard]] virtual WindowId id() const = 0;

        /**
         * @brief Whether this window is still open.
         * @return True until close() has been called.
         */
        [[nodiscard]] virtual bool isOpen() const = 0;

        /**
         * @brief Get the window's current title.
         * @return The title.
         */
        [[nodiscard]] virtual std::string title() const = 0;

        /**
         * @brief Get the size of the window's drawable area.
         * @return The size in pixels.
         */
        [[nodiscard]] virtual Size size() const = 0;

        /**
         * @brief Get the renderer that draws into this window.
         * @return The renderer, valid for as long as this window is.
         */
        [[nodiscard]] virtual IRenderer &renderer() = 0;

        /**
         * @brief Replace the window's title.
         * @param title The new title.
         */
        virtual void setTitle(std::string_view title) = 0;

        /**
         * @brief Close the window.
         *
         * Doing this to an already-closed window is not an error.
         */
        virtual void close() = 0;
    };

} // namespace antwika::gfx
