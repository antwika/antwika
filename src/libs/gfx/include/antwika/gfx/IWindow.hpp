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
         * @brief Get the size this window was created with.
         *
         * The *configured* size: whatever WindowDesc::size asked for,
         * reported back unchanged for as long as the window lives, under
         * every backend, whether or not the window is resizable and
         * whether or not it is still open.
         *
         * This is the size an application lays out and hit-tests
         * against. It is a number the application chose, so it is the
         * same on the machine that recorded a session and on the one
         * replaying it.
         *
         * Alone among IWindow's members this one is not pure, and the
         * default answers with size(). That is exactly right for
         * anything with no window system behind it to disagree with --
         * a test double, or a window that cannot be resized -- and it is
         * what lets an existing implementation stay as it is. Every
         * backend that can be handed a size and then be given a
         * different one overrides it.
         *
         * @return The size in pixels, never zero in either dimension.
         */
        [[nodiscard]] virtual Size configuredSize() const
        {
            return size();
        }

        /**
         * @brief Get the size of the window's drawable area, as the
         * window system currently reports it.
         *
         * Read this as the *reported* size. It is read-only information
         * flowing outwards from the window system, and on a resizable
         * window it changes whenever somebody drags an edge; on a window
         * that is not resizable it may still differ from
         * configuredSize(), because a window manager is free to hand
         * back something other than what was asked for.
         *
         * **Nothing in a simulation may be driven from this.** Feeding
         * it back into the tick loop is what blog/012 rules out: a
         * replay would then resolve recorded input against whatever size
         * the window happened to be, rather than against the size the
         * application chose. Use configuredSize() for layout, for
         * hit-testing, and for anything a replay has to reproduce; use
         * this only to place what is drawn inside the drawable area.
         *
         * A closed window keeps reporting the last size it saw rather
         * than zero, so a caller draining a final frame is not handed a
         * degenerate canvas.
         *
         * @return The size in pixels.
         */
        [[nodiscard]] virtual Size size() const = 0;

        /**
         * @brief Whether this window is currently filling the screen.
         *
         * A window property like the title, not a piece of simulation
         * state: it changes what size() may report and changes nothing
         * else, which is what makes going fullscreen safe for a session
         * being recorded. Nothing a replay reproduces may be a function
         * of it, for the reason size() gives.
         *
         * @return True when the window is fullscreen; false otherwise,
         * and false on a backend with no window system to fill.
         */
        [[nodiscard]] virtual bool isFullscreen() const = 0;

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
         * @brief Make the window fill the screen, or stop filling it.
         *
         * Asking for the state it is already in is not an error, and a
         * backend with no window system honours the request by
         * remembering it and having nothing act on it -- the same
         * headless answer WindowDesc::resizable gets. Doing this to a
         * closed window does nothing.
         *
         * @param fullscreen True to fill the screen, false to restore.
         */
        virtual void setFullscreen(bool fullscreen) = 0;

        /**
         * @brief Close the window.
         *
         * Doing this to an already-closed window is not an error.
         */
        virtual void close() = 0;
    };

} // namespace antwika::gfx
