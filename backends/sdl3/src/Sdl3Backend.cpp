#include "Sdl3Backend.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/log/Level.hpp>

#include "Sdl3Pump.hpp"
#include "Sdl3Window.hpp"

namespace antwika::gfx::sdl3
{

    using antwika::log::Level;

    namespace
    {
        [[noreturn]] void fail(const char *what)
        {
            throw GfxError(
                std::string("gfx.sdl3: ") + what + ": " + SDL_GetError());
        }
    } // namespace

    // The pump raises its own error type, which stops at this seam.
    // Above here, a graphics failure is only ever a GfxError.
    Sdl3Backend::Sdl3Backend(ILogger &logger)
        : logger(logger)
    {
        try
        {
            pump = antwika::sdl3::Sdl3Pump::acquire(logger);
        }
        catch (const antwika::sdl3::Sdl3PumpError &error)
        {
            throw GfxError(std::string("gfx.") + error.what());
        }

        logger.log(Level::Debug, "gfx.sdl3: backend ready");
    }

    Sdl3Backend::~Sdl3Backend() = default;

    std::string_view Sdl3Backend::name() const
    {
        return "sdl3";
    }

    std::size_t Sdl3Backend::maxWindows() const
    {
        return kUnlimitedWindows;
    }

    std::unique_ptr<IWindow> Sdl3Backend::createWindow(const WindowDesc &desc)
    {
        if (desc.size.width == 0 || desc.size.height == 0)
        {
            throw GfxError("gfx.sdl3: window size must have a non-zero "
                           "width and height");
        }

        SDL_Window *window = SDL_CreateWindow(
            desc.title.c_str(),
            static_cast<int>(desc.size.width),
            static_cast<int>(desc.size.height),
            desc.resizable ? SDL_WINDOW_RESIZABLE : 0);

        if (window == nullptr)
        {
            fail("could not create a window");
        }

        SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

        if (renderer == nullptr)
        {
            SDL_DestroyWindow(window);
            fail("could not create a renderer");
        }

        logger.log(Level::Debug, "gfx.sdl3: created window");

        return std::make_unique<Sdl3Window>(
            logger, window, renderer, desc.size);
    }

    std::optional<WindowEvent> Sdl3Backend::pollEvent()
    {
        while (const auto pending = pump->nextWindowEvent())
        {
            const auto &event = *pending;

            // Read event.window only once the type says SDL filled it.
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                return WindowEvent{
                    .window = WindowId{event.window.windowID},
                    .payload = CloseRequested{}};
            }

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                return WindowEvent{
                    .window = WindowId{event.window.windowID},
                    .payload = Resized{
                        .size = {
                            .width = static_cast<std::uint32_t>(
                                event.window.data1),
                            .height = static_cast<std::uint32_t>(
                                event.window.data2)}}};
            }
        }

        return std::nullopt;
    }

} // namespace antwika::gfx::sdl3
