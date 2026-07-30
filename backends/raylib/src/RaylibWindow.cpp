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
    } // namespace

    RaylibWindow::RaylibWindow(
        ILogger &logger,
        RaylibBackend &backend,
        WindowId id,
        const WindowDesc &desc)
        : logger(logger),
          backend(&backend),
          windowId(id),
          windowTitle(desc.title)
    {
        InitWindow(
            static_cast<int>(desc.size.width),
            static_cast<int>(desc.size.height),
            windowTitle.c_str());

        if (!IsWindowReady())
        {
            // InitWindow may have got part way up before giving up.
            // Only CloseWindow releases that.
            CloseWindow();

            throw GfxError("gfx.raylib: could not open the window");
        }

        // Set both ways round, not just when asked for.
        // raylib keeps its window flags in globals outliving a window.
        // One resizable window would make every later one resizable.
        if (desc.resizable)
        {
            SetWindowState(FLAG_WINDOW_RESIZABLE);
        }
        else
        {
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
        }

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

    Size RaylibWindow::size() const
    {
        return open ? currentSize() : lastSize;
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

    void RaylibWindow::close()
    {
        if (!open)
        {
            return;
        }

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

        // Not IsWindowResized(): that stays true until the next poll.
        // Only EndDrawing polls, so draining between frames never ends.
        // The size itself is the latch instead.
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

} // namespace antwika::gfx::raylib
