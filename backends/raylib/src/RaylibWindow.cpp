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
        Size getCurrentSize()
        {
            return Size{
                .width = static_cast<std::uint32_t>(GetScreenWidth()),
                .height = static_cast<std::uint32_t>(GetScreenHeight())};
        }
    }

    RaylibWindow::RaylibWindow(
        ILogger &logger,
        RaylibBackend &backend,
        WindowId idWindow,
        const WindowSpec &spec)
        : logger(logger),
          backend(&backend),
          raylibRenderer(logger),
          windowId(idWindow),
          windowTitle(spec.title),
          requestedSize(spec.size)
    {
        if (spec.hidden)
        {
            SetConfigFlags(FLAG_WINDOW_HIDDEN);
        }

        InitWindow(
            static_cast<int>(spec.size.width),
            static_cast<int>(spec.size.height),
            windowTitle.c_str());

        if (!IsWindowReady())
        {
            throw GfxError(
                "gfx.raylib: could not open a window, so there is no "
                "display to draw on; set DISPLAY, or run under "
                "xvfb-run");
        }

        SetExitKey(KEY_NULL);

        if (spec.hidden)
        {
            SetWindowState(FLAG_WINDOW_HIDDEN);
        }
        else
        {
            ClearWindowState(FLAG_WINDOW_HIDDEN);
        }

        if (spec.resizable)
        {
            SetWindowState(FLAG_WINDOW_RESIZABLE);
        }
        else
        {
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
        }

        setFullscreen(spec.fullscreen);

        lastSize = getCurrentSize();

        logger.log(Level::Debug, "gfx.raylib: created window");
    }

    RaylibWindow::~RaylibWindow()
    {
        close();
    }

    WindowId RaylibWindow::getId() const
    {
        return windowId;
    }

    bool RaylibWindow::isOpen() const
    {
        return open;
    }

    std::string RaylibWindow::getTitle() const
    {
        return windowTitle;
    }

    Size RaylibWindow::getConfiguredSize() const
    {
        return requestedSize;
    }

    Size RaylibWindow::getSize() const
    {
        return open ? getCurrentSize() : lastSize;
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
        lastSize = getCurrentSize();

        raylibRenderer.detach();

        CloseWindow();
        open = false;

        if (backend != nullptr)
        {
            backend->untrackWindow(*this);
        }

        logger.log(Level::Debug, "gfx.raylib: closed window");
    }

    void RaylibWindow::untrackBackend()
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

        const Size nowSize = getCurrentSize();

        if (nowSize != lastSize)
        {
            lastSize = nowSize;

            return WindowEvent{
                .window = windowId,
                .payload = Resized{.size = lastSize}};
        }

        return std::nullopt;
    }

}
