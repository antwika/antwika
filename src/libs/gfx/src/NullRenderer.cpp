#include "NullRenderer.hpp"

#include <memory>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/log/Level.hpp>

#include "NullMesh.hpp"
#include "NullTexture.hpp"
#include "antwika/gfx/GfxError.hpp"

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

    void NullRenderer::drawRect(RectF, Color)
    {
        logger.log(Level::Trace, "gfx.null: draw rect");
    }

    void NullRenderer::drawLine(PointF, PointF, Color)
    {
        logger.log(Level::Trace, "gfx.null: draw line");
    }

    void NullRenderer::drawText(
        PointF, std::string_view, std::uint32_t, Color)
    {
        logger.log(Level::Trace, "gfx.null: draw text");
    }

    std::unique_ptr<ITexture> NullRenderer::createTexture(
        const Bitmap &bitmap)
    {
        if (!bitmap.isComplete())
        {
            throw GfxError(
                "gfx.null: bitmap does not hold the pixels it claims");
        }

        logger.log(Level::Trace, "gfx.null: create texture");

        return std::make_unique<NullTexture>(bitmap.size);
    }

    void NullRenderer::drawTexture(
        const ITexture &, RectF, RectF, Color)
    {
        logger.log(Level::Trace, "gfx.null: draw texture");
    }

    std::unique_ptr<IMesh> NullRenderer::createMesh(
        const MeshData &mesh)
    {
        if (!mesh.isComplete())
        {
            throw GfxError(
                "gfx.null: mesh does not index the vertices it claims");
        }

        logger.log(Level::Trace, "gfx.null: create mesh");

        return std::make_unique<NullMesh>(
            mesh.vertices.size(), mesh.triangleCount());
    }

    void NullRenderer::drawMesh(
        const IMesh &, const Mat4 &, const Camera3D &, Color)
    {
        logger.log(Level::Trace, "gfx.null: draw mesh");
    }

    void NullRenderer::pushTransform(const Mat4 &)
    {
        ++pushed;

        logger.log(Level::Trace, "gfx.null: push transform");
    }

    void NullRenderer::popTransform()
    {
        if (pushed == 0)
        {
            throw GfxError("gfx.null: no transform is pushed");
        }

        --pushed;

        logger.log(Level::Trace, "gfx.null: pop transform");
    }

    void NullRenderer::present()
    {
        logger.log(Level::Trace, "gfx.null: present");
    }

}
