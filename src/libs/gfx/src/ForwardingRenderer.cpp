#include "antwika/gfx/ForwardingRenderer.hpp"

#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

namespace antwika::gfx
{

    ForwardingRenderer::ForwardingRenderer(IRenderer &innerRenderer)
        : wrappedRenderer(innerRenderer)
    {
    }

    IRenderer &ForwardingRenderer::innerRenderer() noexcept
    {
        return wrappedRenderer;
    }

    void ForwardingRenderer::clear(Color color)
    {
        wrappedRenderer.clear(color);
    }

    void ForwardingRenderer::drawRect(RectF rect, Color color)
    {
        wrappedRenderer.drawRect(rect, color);
    }

    void ForwardingRenderer::beginClip(RectF areaRect)
    {
        wrappedRenderer.beginClip(areaRect);
    }

    void ForwardingRenderer::endClip()
    {
        wrappedRenderer.endClip();
    }

    void ForwardingRenderer::drawLine(
        PointF fromPoint, PointF toPoint, Color color)
    {
        wrappedRenderer.drawLine(fromPoint, toPoint, color);
    }

    void ForwardingRenderer::drawText(
        PointF originPoint,
        std::string_view text,
        TextScale scale,
        Color color)
    {
        wrappedRenderer.drawText(originPoint, text, scale, color);
    }

    std::unique_ptr<ITexture> ForwardingRenderer::createTexture(
        const Bitmap &bitmap)
    {
        return wrappedRenderer.createTexture(bitmap);
    }

    void ForwardingRenderer::updateTexture(
        ITexture &texture, const Bitmap &bitmap)
    {
        wrappedRenderer.updateTexture(texture, bitmap);
    }

    void ForwardingRenderer::updateMesh(IMesh &mesh, const MeshData &data)
    {
        wrappedRenderer.updateMesh(mesh, data);
    }

    void ForwardingRenderer::drawTexture(
        const ITexture &texture,
        RectF sourceRect,
        RectF destinationRect,
        Color tintColor)
    {
        wrappedRenderer.drawTexture(
            texture, sourceRect, destinationRect, tintColor);
    }

    std::unique_ptr<IMesh> ForwardingRenderer::createMesh(
        const MeshData &mesh)
    {
        return wrappedRenderer.createMesh(mesh);
    }

    std::unique_ptr<IShader> ForwardingRenderer::createShader(
        const ShaderSource &source)
    {
        return wrappedRenderer.createShader(source);
    }

    void ForwardingRenderer::setShaderNumber(
        const IShader &shader,
        const std::string_view name,
        const float value)
    {
        wrappedRenderer.setShaderNumber(shader, name, value);
    }

    void ForwardingRenderer::setShaderVector(
        const IShader &shader,
        const std::string_view name,
        const Vec3 vector)
    {
        wrappedRenderer.setShaderVector(shader, name, vector);
    }

    std::unique_ptr<IRenderTarget> ForwardingRenderer::createRenderTarget(
        const RenderTargetSpec &spec)
    {
        return wrappedRenderer.createRenderTarget(spec);
    }

    void ForwardingRenderer::beginTarget(
        IRenderTarget &target, const std::optional<Rect> regionRect)
    {
        wrappedRenderer.beginTarget(target, regionRect);
    }

    void ForwardingRenderer::endTarget()
    {
        wrappedRenderer.endTarget();
    }

    void ForwardingRenderer::setShaderMatrix(
        const IShader &shader,
        const std::string_view name,
        const Mat4 &matrix)
    {
        wrappedRenderer.setShaderMatrix(shader, name, matrix);
    }

    void ForwardingRenderer::setShaderColor(
        const IShader &shader,
        const std::string_view name,
        const Color valueColor)
    {
        wrappedRenderer.setShaderColor(shader, name, valueColor);
    }

    void ForwardingRenderer::drawMesh(
        const IMesh &mesh,
        const Mat4 &modelMatrix,
        const Camera3D &camera,
        const MeshMaterial &material)
    {
        wrappedRenderer.drawMesh(mesh, modelMatrix, camera, material);
    }

    void ForwardingRenderer::pushTransform(const Mat4 &transform)
    {
        wrappedRenderer.pushTransform(transform);
    }

    void ForwardingRenderer::popTransform()
    {
        wrappedRenderer.popTransform();
    }

    Bitmap ForwardingRenderer::readPixels()
    {
        return wrappedRenderer.readPixels();
    }

    void ForwardingRenderer::present()
    {
        wrappedRenderer.present();
    }

}
