#include "NullWindow.hpp"

#include <antwika/log/Level.hpp>

namespace antwika::gfx::detail
{

    using antwika::log::Level;

    NullWindow::NullWindow(ILogger &logger, WindowId id, const WindowDesc &desc)
        : logger(logger),
          nullRenderer(logger),
          windowId(id),
          windowTitle(desc.title),
          windowSize(desc.size),
          filling(desc.fullscreen)
    {
    }

    WindowId NullWindow::id() const
    {
        return windowId;
    }

    bool NullWindow::isOpen() const
    {
        return open;
    }

    std::string NullWindow::title() const
    {
        return windowTitle;
    }

    Size NullWindow::configuredSize() const
    {
        return windowSize;
    }

    Size NullWindow::size() const
    {
        return windowSize;
    }

    bool NullWindow::isFullscreen() const
    {
        return filling;
    }

    IRenderer &NullWindow::renderer()
    {
        return nullRenderer;
    }

    void NullWindow::setTitle(std::string_view title)
    {
        windowTitle = title;
    }

    void NullWindow::setFullscreen(bool fullscreen)
    {
        filling = fullscreen;
    }

    void NullWindow::close()
    {
        if (!open)
        {
            return;
        }

        open = false;
        logger.log(Level::Debug, "gfx.null: closed window");
    }

}
