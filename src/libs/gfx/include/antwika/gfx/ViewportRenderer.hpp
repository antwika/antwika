#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/Viewport.hpp"

namespace antwika::gfx
{

    class ViewportRenderer final : public IRenderer
    {
    public:
        ViewportRenderer(IRenderer &inner, Size reported, Size canvas);

        ViewportRenderer(const ViewportRenderer &) = delete;
        ViewportRenderer(ViewportRenderer &&) = delete;

        ViewportRenderer &operator=(const ViewportRenderer &) = delete;
        ViewportRenderer &operator=(ViewportRenderer &&) = delete;

        [[nodiscard]] Viewport viewport() const noexcept;

        /**
         * @brief Rebuilds the transform for a new window size.
         *
         * Ensures: later drawing and viewport() hit-testing use the
         *          new size while the canvas stays unchanged.
         */
        void resize(Size newReported);

        void clear(Color color) override;

        void drawRect(RectF rect, Color color) override;

        void drawLine(PointF from, PointF to, Color color) override;

        void drawText(
            PointF origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        void drawTexture(
            const ITexture &texture,
            RectF source,
            RectF destination,
            Color tint) override;

        [[nodiscard]] std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) override;

        void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) override;

        void pushTransform(const Mat4 &matrix) override;

        void popTransform() override;

        void fillSurround(Color color);

        void present() override;

    private:
        void fillIfDrawable(Rect rect, Color color);

        IRenderer &inner;
        Size reported;
        Size canvas;
        Viewport transform;
    };

}
