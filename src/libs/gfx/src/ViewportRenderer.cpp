#include "antwika/gfx/ViewportRenderer.hpp"

#include <cstdint>

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

    void ViewportRenderer::clear(Color color)
    {
        inner.clear(color);
    }

    void ViewportRenderer::drawRect(Rect rect, Color color)
    {
        inner.drawRect(transform.toWindow(rect), color);
    }

    void ViewportRenderer::drawLine(Point from, Point to, Color color)
    {
        inner.drawLine(
            transform.toWindow(from), transform.toWindow(to), color);
    }

    void ViewportRenderer::drawText(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        inner.drawText(
            transform.toWindow(origin),
            text,
            transform.toWindowScale(scale),
            color);
    }

    std::unique_ptr<ITexture> ViewportRenderer::createTexture(
        const Bitmap &bitmap)
    {
        return inner.createTexture(bitmap);
    }

    void ViewportRenderer::drawTexture(
        const ITexture &texture,
        Rect source,
        Rect destination,
        Color tint)
    {
        inner.drawTexture(
            texture, source, transform.toWindow(destination), tint);
    }

    IRenderer3D *ViewportRenderer::renderer3d()
    {
        return inner.renderer3d();
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

    void ViewportRenderer::present()
    {
        inner.present();
    }

    void ViewportRenderer::fillIfDrawable(Rect rect, Color color)
    {
        // A window the canvas fits exactly has four empty bars.
        // Which is every headless run, and must cost no drawing call.
        if (rect.size.width == 0 || rect.size.height == 0)
        {
            return;
        }

        inner.drawRect(rect, color);
    }

} // namespace antwika::gfx
