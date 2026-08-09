#include "antwika/gfx/BitmapBackend.hpp"

#include <memory>

#include <antwika/log/Level.hpp>

#include "antwika/gfx/BitmapWindow.hpp"
#include "antwika/gfx/GfxError.hpp"

namespace antwika::gfx
{

    using antwika::log::Level;

    BitmapBackend::BitmapBackend(ILogger &logger)
        : logger(logger)
    {
    }

    std::string_view BitmapBackend::name() const
    {
        return "bitmap";
    }

    std::size_t BitmapBackend::maxWindows() const
    {
        return kUnlimitedWindows;
    }

    std::unique_ptr<IWindow> BitmapBackend::createWindow(
        const WindowDesc &desc)
    {
        if (desc.size.width == 0 || desc.size.height == 0)
        {
            throw GfxError("window size must have a non-zero width and "
                           "height");
        }

        logger.log(Level::Debug, "gfx.bitmap: created window");

        const WindowId id{nextWindowId};
        ++nextWindowId;

        return std::make_unique<BitmapWindow>(logger, id, desc);
    }

    std::optional<WindowEvent> BitmapBackend::pollEvent()
    {
        return std::nullopt;
    }

}
