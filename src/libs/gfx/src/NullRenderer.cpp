#include "NullRenderer.hpp"

#include <antwika/log/Level.hpp>

namespace antwika::gfx::detail
{

    using antwika::log::Level;

    NullRenderer::NullRenderer(ILogger &logger)
        : logger(logger)
    {
    }

    void NullRenderer::clear(Color)
    {
        logger.log(Level::Trace, "gfx.null: clear");
    }

    void NullRenderer::drawRect(Rect, Color)
    {
        logger.log(Level::Trace, "gfx.null: draw rect");
    }

    void NullRenderer::present()
    {
        logger.log(Level::Trace, "gfx.null: present");
    }

} // namespace antwika::gfx::detail
