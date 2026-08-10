#include "antwika/gfx/ViewportRenderer.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Glyphs.hpp"

namespace antwika::gfx
{

    ViewportRenderer::ViewportRenderer(
        IRenderer &inner, Size reported, Size canvas)
        : inner(inner),
          reported(reported),
          canvas(canvas),
          transform(viewportFor(reported, canvas))
    {
    }

    Viewport ViewportRenderer::viewport() const noexcept
    {
        return transform;
    }

    void ViewportRenderer::resize(const Size newReported)
    {
        reported = newReported;
        transform = viewportFor(newReported, canvas);
    }

    void ViewportRenderer::clear(Color color)
    {
        inner.clear(color);
    }

    void ViewportRenderer::drawRect(RectF rect, Color color)
    {
        inner.drawRect(transform.toWindow(rect), color);
    }

    void ViewportRenderer::drawLine(
        PointF from, PointF to, Color color)
    {
        inner.drawLine(
            transform.toWindow(from), transform.toWindow(to), color);
    }

    void ViewportRenderer::drawText(
        PointF origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        const auto multiplier = textMultiplierOf(scale);
        const auto drawn = encodeTextScale(
            textFaceOf(scale), transform.toWindowScale(multiplier));

        const bool exact =
            (static_cast<std::uint64_t>(multiplier)
             * transform.numerator)
                % transform.denominator
            == 0;

        if (exact)
        {
            inner.drawText(transform.toWindow(origin), text, drawn, color);
            return;
        }

        for (std::size_t at = 0; at < text.size(); ++at)
        {
            const auto step = static_cast<std::int64_t>(at)
                              * scaledGlyphAdvance(scale);

            const PointF cell{
                origin.x + static_cast<float>(step), origin.y};

            inner.drawText(
                transform.toWindow(cell), text.substr(at, 1), drawn, color);
        }
    }

    std::unique_ptr<ITexture> ViewportRenderer::createTexture(
        const Bitmap &bitmap)
    {
        return inner.createTexture(bitmap);
    }

    void ViewportRenderer::drawTexture(
        const ITexture &texture,
        RectF source,
        RectF destination,
        Color tint)
    {
        inner.drawTexture(
            texture, source, transform.toWindow(destination), tint);
    }

    std::unique_ptr<IMesh> ViewportRenderer::createMesh(
        const MeshData &mesh)
    {
        return inner.createMesh(mesh);
    }

    void ViewportRenderer::drawMesh(
        const IMesh &mesh,
        const Mat4 &model,
        const Camera3D &camera,
        const Color tint)
    {
        inner.drawMesh(mesh, model, camera, tint);
    }

    void ViewportRenderer::fillSurround(Color color)
    {
        const auto frame = transform.frame(canvas);

        const auto right = frame.origin.x
                           + static_cast<std::int32_t>(frame.size.width);
        const auto bottom = frame.origin.y
                            + static_cast<std::int32_t>(frame.size.height);

        const auto width = static_cast<std::int32_t>(reported.width);
        const auto height = static_cast<std::int32_t>(reported.height);

        fillIfDrawable(
            Rect{
                .origin = Point{.x = 0, .y = 0},
                .size = {
                    .width = static_cast<std::uint32_t>(frame.origin.x),
                    .height = reported.height}},
            color);

        fillIfDrawable(
            Rect{
                .origin = Point{.x = right, .y = 0},
                .size = {
                    .width = static_cast<std::uint32_t>(width - right),
                    .height = reported.height}},
            color);

        fillIfDrawable(
            Rect{
                .origin = Point{.x = frame.origin.x, .y = 0},
                .size = {
                    .width = frame.size.width,
                    .height = static_cast<std::uint32_t>(frame.origin.y)}},
            color);

        fillIfDrawable(
            Rect{
                .origin = Point{.x = frame.origin.x, .y = bottom},
                .size = {
                    .width = frame.size.width,
                    .height = static_cast<std::uint32_t>(height - bottom)}},
            color);
    }

    void ViewportRenderer::pushTransform(const Mat4 &matrix)
    {
        inner.pushTransform(matrix);
    }

    void ViewportRenderer::popTransform()
    {
        inner.popTransform();
    }

    void ViewportRenderer::present()
    {
        inner.present();
    }

    void ViewportRenderer::fillIfDrawable(Rect rect, Color color)
    {
        if (rect.size.width == 0 || rect.size.height == 0)
        {
            return;
        }

        inner.drawRect(rect, color);
    }

}
