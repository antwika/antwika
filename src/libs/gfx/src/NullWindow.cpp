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
          windowSize(desc.size)
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

    Size NullWindow::size() const
    {
        return windowSize;
    }

    IRenderer &NullWindow::renderer()
    {
        return nullRenderer;
    }

    void NullWindow::setTitle(std::string_view title)
    {
        windowTitle = title;
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

} // namespace antwika::gfx::detail
