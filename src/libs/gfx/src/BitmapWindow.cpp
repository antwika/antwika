#include "antwika/gfx/BitmapWindow.hpp"

#include <antwika/log/Level.hpp>

namespace antwika::gfx
{

    using antwika::log::Level;

    BitmapWindow::BitmapWindow(
        ILogger &logger, WindowId id, const WindowDesc &desc)
        : logger(logger),
          bitmapRenderer(logger, desc.size),
          windowId(id),
          windowTitle(desc.title),
          windowSize(desc.size),
          filling(desc.fullscreen)
    {
    }

    WindowId BitmapWindow::id() const
    {
        return windowId;
    }

    bool BitmapWindow::isOpen() const
    {
        return open;
    }

    std::string BitmapWindow::title() const
    {
        return windowTitle;
    }

    Size BitmapWindow::configuredSize() const
    {
        return windowSize;
    }

    Size BitmapWindow::size() const
    {
        return windowSize;
    }

    bool BitmapWindow::isFullscreen() const
    {
        return filling;
    }

    IRenderer &BitmapWindow::renderer()
    {
        return bitmapRenderer;
    }

    const Bitmap &BitmapWindow::page() const noexcept
    {
        return bitmapRenderer.page();
    }

    void BitmapWindow::setTitle(std::string_view title)
    {
        windowTitle = title;
    }

    void BitmapWindow::setFullscreen(bool fullscreen)
    {
        filling = fullscreen;
    }

    void BitmapWindow::close()
    {
        if (!open)
        {
            return;
        }

        open = false;
        logger.log(Level::Debug, "gfx.bitmap: closed window");
    }

}
