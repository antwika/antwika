#include "NullRenderer.hpp"

#include <memory>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/log/Level.hpp>

#include "NullMesh.hpp"
#include "NullShader.hpp"
#include "NullTexture.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "NullRenderTarget.hpp"

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

    void NullRenderer::beginClip(RectF)
    {
        logger.log(Level::Trace, "gfx.null: begin clip");
    }

    void NullRenderer::endClip()
    {
        logger.log(Level::Trace, "gfx.null: end clip");
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
        if (!bitmap.isValid())
        {
            throw GfxError(
                "gfx.null: bitmap does not hold the pixels it claims");
        }

        logger.log(Level::Trace, "gfx.null: create texture");

        return std::make_unique<NullTexture>(bitmap.size);
    }

    void NullRenderer::updateTexture(ITexture &, const Bitmap &)
    {
        logger.log(Level::Trace, "gfx.null: update texture");
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

    std::unique_ptr<IShader> NullRenderer::createShader(
        const ShaderSource &source)
    {
        if (!source.isComplete())
        {
            throw GfxError(
                "gfx.null: shader source is missing a stage");
        }

        logger.log(Level::Trace, "gfx.null: create shader");

        return std::make_unique<NullShader>();
    }

    void NullRenderer::setShaderNumber(
        const IShader &, std::string_view, float)
    {
        logger.log(Level::Trace, "gfx.null: set shader number");
    }

    std::unique_ptr<IRenderTarget> NullRenderer::createRenderTarget(
        const RenderTargetSpec &spec)
    {
        if (spec.size.width == 0 || spec.size.height == 0)
        {
            throw GfxError(
                "gfx.null: a render target needs a size");
        }

        logger.log(Level::Trace, "gfx.null: create render target");

        return std::make_unique<NullRenderTarget>(spec);
    }

    void NullRenderer::beginTargetRegion(IRenderTarget &, const Rect)
    {
    }

    void NullRenderer::beginTarget(IRenderTarget &)
    {
        logger.log(Level::Trace, "gfx.null: begin target");
    }

    void NullRenderer::endTarget()
    {
        logger.log(Level::Trace, "gfx.null: end target");
    }

    void NullRenderer::setShaderMatrix(
        const IShader &, std::string_view, const Mat4 &)
    {
        logger.log(Level::Trace, "gfx.null: set shader matrix");
    }

    void NullRenderer::setShaderVector(
        const IShader &, std::string_view, Vec3)
    {
        logger.log(Level::Trace, "gfx.null: set shader vector");
    }

    void NullRenderer::setShaderColor(
        const IShader &, std::string_view, Color)
    {
        logger.log(Level::Trace, "gfx.null: set shader color");
    }

    void NullRenderer::drawMesh(
        const IMesh &, const Mat4 &, const Camera3D &, const MeshMaterial &)
    {
        logger.log(Level::Trace, "gfx.null: draw mesh");
    }

    void NullRenderer::pushTransform(const Mat4 &)
    {
        ++pushedCount;

        logger.log(Level::Trace, "gfx.null: push transform");
    }

    void NullRenderer::popTransform()
    {
        if (pushedCount == 0)
        {
            throw GfxError("gfx.null: no transform is pushed");
        }

        --pushedCount;

        logger.log(Level::Trace, "gfx.null: pop transform");
    }

    Bitmap NullRenderer::readPixels()
    {
        logger.log(Level::Trace, "gfx.null: read pixels");

        return Bitmap{};
    }

    void NullRenderer::present()
    {
        logger.log(Level::Trace, "gfx.null: present");
    }

}
