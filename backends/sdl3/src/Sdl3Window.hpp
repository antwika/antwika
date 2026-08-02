#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <string_view>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/log/ILogger.hpp>

#include "Sdl3Renderer.hpp"

namespace antwika::gfx::sdl3
{

    using antwika::log::ILogger;

    /**
     * @brief One SDL window, and the SDL renderer that draws into it.
     *
     * SDL has no notion of a window that is open but not shown, so
     * close() destroys the underlying window outright. The last known id,
     * title and size are kept so this object can still answer questions
     * about itself afterwards, which IWindow's contract requires.
     */
    class Sdl3Window final : public IWindow
    {
    public:
        /**
         * @brief Take ownership of an SDL window and its renderer.
         * @param logger Receives this window's diagnostics.
         * @param window The SDL window, already created.
         * @param renderer The SDL renderer for that window.
         * @param configured The size WindowDesc asked for, which is not
         * necessarily the size SDL went on to create.
         */
        Sdl3Window(
            ILogger &logger,
            SDL_Window *window,
            SDL_Renderer *renderer,
            Size configured);

        Sdl3Window(const Sdl3Window &) = delete;
        Sdl3Window(Sdl3Window &&) = delete;

        Sdl3Window &operator=(const Sdl3Window &) = delete;
        Sdl3Window &operator=(Sdl3Window &&) = delete;

        /**
         * @brief Destroy the SDL renderer and window, if still open.
         */
        ~Sdl3Window() override;

        /**
         * @brief Get this window's id.
         * @return The SDL window id captured at creation.
         */
        [[nodiscard]] WindowId id() const override;

        /**
         * @brief Whether this window is still open.
         * @return True until close() has been called.
         */
        [[nodiscard]] bool isOpen() const override;

        /**
         * @brief Get the window's current title.
         * @return The title SDL reports, or the last one seen if closed.
         */
        [[nodiscard]] std::string title() const override;

        /**
         * @brief Get the size the window was created with.
         * @return WindowDesc::size, unchanged by anything SDL or a
         * window manager later does to the window.
         */
        [[nodiscard]] Size configuredSize() const override;

        /**
         * @brief Get the size of the window's drawable area.
         * @return The size SDL reports, or the last one seen if closed.
         * On a resizable window this follows the user around.
         */
        [[nodiscard]] Size size() const override;

        /**
         * @brief Whether this window is filling the screen.
         * @return What SDL's window flags say, or the last state seen
         * if closed.
         */
        [[nodiscard]] bool isFullscreen() const override;

        /**
         * @brief Get the renderer that draws into this window.
         * @return The renderer, which discards everything once closed.
         */
        [[nodiscard]] IRenderer &renderer() override;

        /**
         * @brief Replace the window's title.
         * @param title The new title.
         */
        void setTitle(std::string_view title) override;

        /**
         * @brief Make the window fill the screen, or stop filling it.
         * @param fullscreen True to fill the screen, false to restore.
         */
        void setFullscreen(bool fullscreen) override;

        /**
         * @brief Destroy the window, or do nothing if already closed.
         */
        void close() override;

    private:
        ILogger &logger;
        Sdl3Renderer sdlRenderer;
        SDL_Window *window;
        SDL_Renderer *rawRenderer;
        WindowId windowId;
        std::string lastTitle;
        Size requestedSize;
        Size lastSize;
        bool lastFullscreen = false;
    };

} // namespace antwika::gfx::sdl3
