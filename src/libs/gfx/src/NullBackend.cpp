#include "antwika/gfx/NullBackend.hpp"

#include <memory>

#include <antwika/log/Level.hpp>

#include "antwika/gfx/GfxError.hpp"

#include "NullWindow.hpp"

namespace antwika::gfx
{

    using antwika::log::Level;

    NullBackend::NullBackend(ILogger &logger)
        : logger(logger)
    {
    }

    std::string_view NullBackend::name() const
    {
        return "null";
    }

    std::size_t NullBackend::maxWindows() const
    {
        return kUnlimitedWindows;
    }

    std::unique_ptr<IWindow> NullBackend::createWindow(const WindowDesc &desc)
    {
        if (desc.size.width == 0 || desc.size.height == 0)
        {
            throw GfxError("window size must have a non-zero width and "
                           "height");
        }

        logger.log(Level::Debug, "gfx.null: created window");

        const WindowId id{nextWindowId};
        ++nextWindowId;

        return std::make_unique<detail::NullWindow>(logger, id, desc);
    }

    std::optional<WindowEvent> NullBackend::pollEvent()
    {
        return std::nullopt;
    }

}
