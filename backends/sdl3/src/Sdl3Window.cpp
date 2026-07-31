#include "Sdl3Window.hpp"

#include <antwika/log/Level.hpp>

namespace antwika::gfx::sdl3
{

    using antwika::log::Level;

    namespace
    {
        Size readSize(SDL_Window *window)
        {
            int width = 0;
            int height = 0;

            if (!SDL_GetWindowSizeInPixels(window, &width, &height))
            {
                return Size{};
            }

            return Size{
                .width = static_cast<std::uint32_t>(width),
                .height = static_cast<std::uint32_t>(height)};
        }

        std::string readTitle(SDL_Window *window)
        {
            const char *title = SDL_GetWindowTitle(window);

            return title == nullptr ? std::string{} : std::string(title);
        }
    } // namespace

    Sdl3Window::Sdl3Window(
        ILogger &logger,
        SDL_Window *window,
        SDL_Renderer *renderer,
        Size configured)
        : logger(logger),
          sdlRenderer(logger, renderer),
          window(window),
          rawRenderer(renderer),
          windowId(WindowId{SDL_GetWindowID(window)}),
          lastTitle(readTitle(window)),
          requestedSize(configured),
          lastSize(readSize(window))
    {
    }

    Sdl3Window::~Sdl3Window()
    {
        close();
    }

    WindowId Sdl3Window::id() const
    {
        return windowId;
    }

    bool Sdl3Window::isOpen() const
    {
        return window != nullptr;
    }

    std::string Sdl3Window::title() const
    {
        return window == nullptr ? lastTitle : readTitle(window);
    }

    Size Sdl3Window::configuredSize() const
    {
        return requestedSize;
    }

    Size Sdl3Window::size() const
    {
        return window == nullptr ? lastSize : readSize(window);
    }

    IRenderer &Sdl3Window::renderer()
    {
        return sdlRenderer;
    }

    void Sdl3Window::setTitle(std::string_view title)
    {
        lastTitle = std::string(title);

        if (window == nullptr)
        {
            return;
        }

        if (!SDL_SetWindowTitle(window, lastTitle.c_str()))
        {
            logger.log(
                Level::Warning,
                std::string("gfx.sdl3: could not set the window title: ") +
                    SDL_GetError());
        }
    }

    void Sdl3Window::close()
    {
        if (window == nullptr)
        {
            return;
        }

        lastTitle = readTitle(window);
        lastSize = readSize(window);

        // Before SDL_DestroyRenderer, load-bearing for textures.
        // Freeing a texture after its renderer is undefined.
        sdlRenderer.detach();

        SDL_DestroyRenderer(rawRenderer);
        rawRenderer = nullptr;

        SDL_DestroyWindow(window);
        window = nullptr;

        logger.log(Level::Debug, "gfx.sdl3: closed window");
    }

} // namespace antwika::gfx::sdl3
