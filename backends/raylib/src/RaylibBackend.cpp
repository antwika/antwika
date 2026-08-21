#include "RaylibBackend.hpp"

#include <raylib.h>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/WindowId.hpp>

#include "RaylibWindow.hpp"

namespace antwika::gfx::raylib
{

    RaylibBackend::RaylibBackend(ILogger &logger)
        : logger(logger)
    {
        SetTraceLogLevel(LOG_WARNING);
    }

    RaylibBackend::~RaylibBackend()
    {
        if (openWindow != nullptr)
        {
            openWindow->untrackBackend();
        }
    }

    std::string_view RaylibBackend::name() const
    {
        return "raylib";
    }

    std::size_t RaylibBackend::maxWindows() const
    {
        return 1;
    }

    std::unique_ptr<IWindow> RaylibBackend::createWindow(
        const WindowSpec &spec)
    {
        if (spec.size.width == 0 || spec.size.height == 0)
        {
            throw GfxError("gfx.raylib: window size must have a non-zero "
                           "width and height");
        }

        if (openWindow != nullptr)
        {
            throw GfxError("gfx.raylib: only one window can be open at a "
                           "time");
        }

        const WindowId windowId{nextWindowId};
        ++nextWindowId;

        auto window =
            std::make_unique<RaylibWindow>(logger, *this, windowId, spec);

        openWindow = window.get();

        return window;
    }

    std::optional<WindowEvent> RaylibBackend::pollEvent()
    {
        if (openWindow == nullptr)
        {
            return std::nullopt;
        }

        return openWindow->takePendingEvent();
    }

    void RaylibBackend::untrackWindow(const RaylibWindow &window)
    {
        if (openWindow == &window)
        {
            openWindow = nullptr;
        }
    }

}
