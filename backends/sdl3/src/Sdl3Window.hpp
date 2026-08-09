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

    class Sdl3Window final : public IWindow
    {
    public:
        Sdl3Window(
            ILogger &logger,
            SDL_Window *window,
            SDL_Renderer *renderer,
            Size configured);

        Sdl3Window(const Sdl3Window &) = delete;
        Sdl3Window(Sdl3Window &&) = delete;

        Sdl3Window &operator=(const Sdl3Window &) = delete;
        Sdl3Window &operator=(Sdl3Window &&) = delete;

        ~Sdl3Window() override;

        [[nodiscard]] WindowId id() const override;

        [[nodiscard]] bool isOpen() const override;

        [[nodiscard]] std::string title() const override;

        [[nodiscard]] Size configuredSize() const override;

        [[nodiscard]] Size size() const override;

        [[nodiscard]] bool isFullscreen() const override;

        [[nodiscard]] IRenderer &renderer() override;

        void setTitle(std::string_view title) override;

        void setFullscreen(bool fullscreen) override;

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

}
