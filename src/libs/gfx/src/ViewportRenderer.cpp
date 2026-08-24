#include "antwika/gfx/ViewportRenderer.hpp"

#include <cstddef>
#include <cstdint>
#include <variant>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Glyphs.hpp"

namespace antwika::gfx
{

    ViewportRenderer::ViewportRenderer(
        IRenderer &innerRenderer,
        Size reportedSize,
        Size canvasSize,
        const Fit fit)
        : innerRenderer(innerRenderer),
          reportedSize(reportedSize),
          canvasSize(canvasSize),
          fit(fit),
          transformViewport(viewportFor(reportedSize, canvasSize, fit))
    {
    }

    bool ViewportRenderer::isTargetBound() const noexcept
    {
        return bound > 0;
    }

    Viewport ViewportRenderer::getViewport() const noexcept
    {
        return transformViewport;
    }

    IRenderer &ViewportRenderer::nativeRenderer() noexcept
    {
        return innerRenderer;
    }

    Size ViewportRenderer::getWindowSize() const noexcept
    {
        return reportedSize;
    }

    void ViewportRenderer::resize(const Size newReportedSize)
    {
        reportedSize = newReportedSize;
        transformViewport = viewportFor(newReportedSize, canvasSize, fit);
    }

    void ViewportRenderer::clear(Color color)
    {
        innerRenderer.clear(color);
    }

    void ViewportRenderer::drawRect(RectF rect, Color color)
    {
        innerRenderer.drawRect(
            isTargetBound() ? rect : transformViewport.toWindow(rect),
            color);
    }

    void ViewportRenderer::beginClip(RectF areaRect)
    {
        innerRenderer.beginClip(
            isTargetBound() ? areaRect : transformViewport.toWindow(areaRect));
    }

    void ViewportRenderer::endClip()
    {
        innerRenderer.endClip();
    }

    void ViewportRenderer::drawLine(
        PointF fromPoint, PointF toPoint, Color color)
    {
        if (isTargetBound())
        {
            innerRenderer.drawLine(fromPoint, toPoint, color);

            return;
        }

        innerRenderer.drawLine(
            transformViewport.toWindow(
                fromPoint), transformViewport.toWindow(toPoint), color);
    }

    void ViewportRenderer::drawText(
        PointF originPoint,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        if (isTargetBound())
        {
            innerRenderer.drawText(originPoint, text, scale, color);

            return;
        }

        const auto multiplier = textMultiplierOf(scale);
        const auto encodedScale = getEncodeTextScale(
            textFaceOf(scale), transformViewport.toWindowScale(multiplier));

        const bool exact =
            (static_cast<std::uint64_t>(multiplier)
             * transformViewport.numerator)
                % transformViewport.denominator
            == 0;

        if (exact)
        {
            innerRenderer.drawText(
                transformViewport.toWindow(originPoint),
                text,
                encodedScale,
                color);
            return;
        }

        for (std::size_t index = 0; index < text.size(); ++index)
        {
            const auto step = static_cast<std::int64_t>(index)
                              * getScaledGlyphAdvance(scale);

            const PointF cellPoint{
                originPoint.x + static_cast<float>(step), originPoint.y};

            innerRenderer.drawText(
                transformViewport.toWindow(cellPoint),
                text.substr(index, 1),
                encodedScale,
                color);
        }
    }

    void ViewportRenderer::updateMesh(IMesh &mesh, const MeshData &data)
    {
        innerRenderer.updateMesh(mesh, data);
    }

    void ViewportRenderer::updateTexture(
        ITexture &texture, const Bitmap &bitmap)
    {
        innerRenderer.updateTexture(texture, bitmap);
    }

    std::unique_ptr<ITexture> ViewportRenderer::createTexture(
        const Bitmap &bitmap)
    {
        return innerRenderer.createTexture(bitmap);
    }

    void ViewportRenderer::drawTexture(
        const ITexture &texture,
        RectF sourceRect,
        RectF destinationRect,
        Color tintColor)
    {
        innerRenderer.drawTexture(
            texture,
            sourceRect,
            isTargetBound()
                ? destinationRect
                : transformViewport.toWindow(destinationRect),
            tintColor);
    }

    std::unique_ptr<IMesh> ViewportRenderer::createMesh(
        const MeshData &mesh)
    {
        return innerRenderer.createMesh(mesh);
    }

    std::unique_ptr<IShader> ViewportRenderer::createShader(
        const ShaderSource &source)
    {
        return innerRenderer.createShader(source);
    }

    void ViewportRenderer::setShaderNumber(
        const IShader &shader,
        const std::string_view name,
        const float value)
    {
        innerRenderer.setShaderNumber(shader, name, value);
    }

    void ViewportRenderer::setShaderVector(
        const IShader &shader,
        const std::string_view name,
        const Vec3 vector)
    {
        innerRenderer.setShaderVector(shader, name, vector);
    }

    std::unique_ptr<IRenderTarget> ViewportRenderer::createRenderTarget(
        const RenderTargetSpec &spec)
    {
        return innerRenderer.createRenderTarget(spec);
    }

    void ViewportRenderer::beginTarget(IRenderTarget &target)
    {
        ++bound;
        innerRenderer.beginTarget(target);
    }

    void ViewportRenderer::beginTargetRegion(
        IRenderTarget &target, const Rect regionRect)
    {
        ++bound;
        innerRenderer.beginTargetRegion(target, regionRect);
    }

    void ViewportRenderer::endTarget()
    {
        if (bound > 0)
        {
            --bound;
        }

        innerRenderer.endTarget();
    }

    void ViewportRenderer::setShaderMatrix(
        const IShader &shader,
        const std::string_view name,
        const Mat4 &matrix)
    {
        innerRenderer.setShaderMatrix(shader, name, matrix);
    }

    void ViewportRenderer::setShaderColor(
        const IShader &shader,
        const std::string_view name,
        const Color valueColor)
    {
        innerRenderer.setShaderColor(shader, name, valueColor);
    }

    void ViewportRenderer::drawMesh(
        const IMesh &mesh,
        const Mat4 &modelMatrix,
        const Camera3D &camera,
        const MeshMaterial &material)
    {
        innerRenderer.drawMesh(
            mesh,
            modelMatrix,
            isTargetBound() ? camera : getOnWindow(camera),
            material);
    }

    Camera3D ViewportRenderer::getOnWindow(const Camera3D &camera) const
    {
        const auto *how =
            std::get_if<Orthographic>(&camera.getProjection());

        if (how == nullptr)
        {
            return camera;
        }

        const auto windowWidth = static_cast<float>(reportedSize.width);
        const auto windowHeight = static_cast<float>(reportedSize.height);
        const auto canvasWidth = static_cast<float>(canvasSize.width);
        const auto canvasHeight = static_cast<float>(canvasSize.height);

        const auto scale =
            static_cast<float>(transformViewport.numerator)
            / static_cast<float>(transformViewport.denominator);

        const float heldWidth = windowWidth / scale;
        const float heldHeight = windowHeight / scale;

        const float slideX =
            ((windowWidth / 2.0F)
             - static_cast<float>(transformViewport.offsetPoint.x))
                / scale
            - (canvasWidth / 2.0F);
        const float slideY =
            ((windowHeight / 2.0F)
             - static_cast<float>(transformViewport.offsetPoint.y))
                / scale
            - (canvasHeight / 2.0F);

        Camera3D placedCamera = camera;

        placedCamera.setProjection(
            Orthographic{ // GCOVR_EXCL_LINE
                .halfWidth = how->halfWidth * heldWidth / canvasWidth,
                .halfHeight = how->halfHeight * heldHeight / canvasHeight,
                .offsetX = how->offsetX
                           + slideX * 2.0F * how->halfWidth / canvasWidth,
                .offsetY = how->offsetY
                           - slideY * 2.0F * how->halfHeight
                                 / canvasHeight,
                .nearPlane = how->nearPlane,
                .farPlane = how->farPlane});

        return placedCamera;
    }

    void ViewportRenderer::fillLetterbox(Color color)
    {
        const auto frame = transformViewport.getFrame(canvasSize);

        const auto right = frame.originPoint.x
                           + static_cast<std::int32_t>(frame.size.width);
        const auto bottom = frame.originPoint.y
                            + static_cast<std::int32_t>(frame.size.height);

        const auto width = static_cast<std::int32_t>(reportedSize.width);
        const auto height = static_cast<std::int32_t>(reportedSize.height);

        fillIfDrawable(
            Rect{
                .originPoint = Point{.x = 0, .y = 0},
                .size = {
                    .width = static_cast<std::uint32_t>(frame.originPoint.x),
                    .height = reportedSize.height}},
            color);

        fillIfDrawable(
            Rect{
                .originPoint = Point{.x = right, .y = 0},
                .size = {
                    .width = static_cast<std::uint32_t>(width - right),
                    .height = reportedSize.height}},
            color);

        fillIfDrawable(
            Rect{
                .originPoint = Point{.x = frame.originPoint.x, .y = 0},
                .size = {
                    .width = frame.size.width,
                    .height = static_cast<std::uint32_t>(frame.originPoint.y)}},
            color);

        fillIfDrawable(
            Rect{
                .originPoint = Point{.x = frame.originPoint.x, .y = bottom},
                .size = {
                    .width = frame.size.width,
                    .height = static_cast<std::uint32_t>(height - bottom)}},
            color);
    }

    void ViewportRenderer::pushTransform(const Mat4 &matrix)
    {
        innerRenderer.pushTransform(matrix);
    }

    void ViewportRenderer::popTransform()
    {
        innerRenderer.popTransform();
    }

    Bitmap ViewportRenderer::readPixels()
    {
        return innerRenderer.readPixels();
    }

    void ViewportRenderer::present()
    {
        innerRenderer.present();
    }

    void ViewportRenderer::fillIfDrawable(Rect rect, Color color)
    {
        if (rect.size.width == 0 || rect.size.height == 0)
        {
            return;
        }

        innerRenderer.drawRect(rect, color);
    }

}
