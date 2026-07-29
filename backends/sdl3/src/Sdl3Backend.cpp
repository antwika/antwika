#include "Sdl3Backend.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/log/Level.hpp>

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

    Sdl3Backend::Sdl3Backend(ILogger &logger)
        : logger(logger)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            fail("could not initialise the video subsystem");
        }

        logger.log(Level::Info, "gfx.sdl3: video subsystem started");
    }

    Sdl3Backend::~Sdl3Backend()
    {
        SDL_Quit();
    }

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
            0);

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

        return std::make_unique<Sdl3Window>(logger, window, renderer);
    }

    std::optional<WindowEvent> Sdl3Backend::pollEvent()
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            const WindowId window{event.window.windowID};

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                return WindowEvent{
                    .window = window,
                    .payload = CloseRequested{}};
            }

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                return WindowEvent{
                    .window = window,
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
