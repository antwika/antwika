#include "RaylibWindow.hpp"

#include <raylib.h>

#include <cstdint>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/log/Level.hpp>

#include "RaylibBackend.hpp"

namespace antwika::gfx::raylib
{

    using antwika::log::Level;

    namespace
    {
        Size currentSize()
        {
            return Size{
                .width = static_cast<std::uint32_t>(GetScreenWidth()),
                .height = static_cast<std::uint32_t>(GetScreenHeight())};
        }
    }

    RaylibWindow::RaylibWindow(
        ILogger &logger,
        RaylibBackend &backend,
        WindowId id,
        const WindowDesc &desc)
        : logger(logger),
          backend(&backend),
          windowId(id),
          windowTitle(desc.title),
          requestedSize(desc.size)
    {
        InitWindow(
            static_cast<int>(desc.size.width),
            static_cast<int>(desc.size.height),
            windowTitle.c_str());

        if (!IsWindowReady())
        {
            throw GfxError(
                "gfx.raylib: could not open a window, so there is no "
                "display to draw on; set DISPLAY, or run under "
                "xvfb-run");
        }

        SetExitKey(KEY_NULL);

        if (desc.resizable)
        {
            SetWindowState(FLAG_WINDOW_RESIZABLE);
        }
        else
        {
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
        }

        setFullscreen(desc.fullscreen);

        lastSize = currentSize();

        logger.log(Level::Debug, "gfx.raylib: created window");
    }

    RaylibWindow::~RaylibWindow()
    {
        close();
    }

    WindowId RaylibWindow::id() const
    {
        return windowId;
    }

    bool RaylibWindow::isOpen() const
    {
        return open;
    }

    std::string RaylibWindow::title() const
    {
        return windowTitle;
    }

    Size RaylibWindow::configuredSize() const
    {
        return requestedSize;
    }

    Size RaylibWindow::size() const
    {
        return open ? currentSize() : lastSize;
    }

    bool RaylibWindow::isFullscreen() const
    {
        return open ? IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)
                    : lastFullscreen;
    }

    IRenderer &RaylibWindow::renderer()
    {
        return raylibRenderer;
    }

    void RaylibWindow::setTitle(std::string_view title)
    {
        windowTitle = std::string(title);

        if (!open)
        {
            return;
        }

        SetWindowTitle(windowTitle.c_str());
    }

    void RaylibWindow::setSize(const Size size)
    {
        requestedSize = size;

        if (!open)
        {
            return;
        }

        SetWindowSize(
            static_cast<int>(size.width),
            static_cast<int>(size.height));
    }

    void RaylibWindow::setFullscreen(bool fullscreen)
    {
        lastFullscreen = fullscreen;

        if (!open)
        {
            return;
        }

        if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)
            != fullscreen)
        {
            ToggleBorderlessWindowed();
        }
    }

    void RaylibWindow::close()
    {
        if (!open)
        {
            return;
        }

        lastFullscreen = IsWindowFullscreen();
        lastSize = currentSize();

        raylibRenderer.detach();

        CloseWindow();
        open = false;

        if (backend != nullptr)
        {
            backend->forgetWindow(*this);
        }

        logger.log(Level::Debug, "gfx.raylib: closed window");
    }

    void RaylibWindow::forgetBackend()
    {
        backend = nullptr;
    }

    std::optional<WindowEvent> RaylibWindow::takePendingEvent()
    {
        if (!open)
        {
            return std::nullopt;
        }

        if (!closeReported && WindowShouldClose())
        {
            closeReported = true;

            return WindowEvent{
                .window = windowId,
                .payload = CloseRequested{}};
        }

        const Size current = currentSize();

        if (current != lastSize)
        {
            lastSize = current;

            return WindowEvent{
                .window = windowId,
                .payload = Resized{.size = lastSize}};
        }

        return std::nullopt;
    }

}
