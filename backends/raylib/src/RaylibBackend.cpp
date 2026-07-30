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
        // raylib logs to stderr, and its only hook is a C callback.
        // Reaching an ILogger from there would need a global.
        // So just quieten anything below a real problem.
        SetTraceLogLevel(LOG_WARNING);
    }

    RaylibBackend::~RaylibBackend()
    {
        if (live != nullptr)
        {
            live->forgetBackend();
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
        const WindowDesc &desc)
    {
        if (desc.size.width == 0 || desc.size.height == 0)
        {
            throw GfxError("gfx.raylib: window size must have a non-zero "
                           "width and height");
        }

        if (live != nullptr)
        {
            throw GfxError("gfx.raylib: only one window can be open at a "
                           "time");
        }

        const WindowId id{nextWindowId};
        ++nextWindowId;

        auto window =
            std::make_unique<RaylibWindow>(logger, *this, id, desc);

        live = window.get();

        return window;
    }

    std::optional<WindowEvent> RaylibBackend::pollEvent()
    {
        if (live == nullptr)
        {
            return std::nullopt;
        }

        return live->takePendingEvent();
    }

    void RaylibBackend::forgetWindow(const RaylibWindow &window)
    {
        if (live == &window)
        {
            live = nullptr;
        }
    }

} // namespace antwika::gfx::raylib
