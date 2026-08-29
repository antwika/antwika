#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/ForwardingRenderer.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/MeshMaterial.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/Viewport.hpp"

namespace antwika::gfx
{

    class ViewportRenderer final : public ForwardingRenderer
    {
    public:
        ViewportRenderer(
            IRenderer &innerRenderer,
            Size reportedSize,
            Size canvasSize,
            Fit fit = Fit::Stretched);

        [[nodiscard]] Viewport getViewport() const noexcept;

        using ForwardingRenderer::innerRenderer;

        [[nodiscard]] Size getWindowSize() const noexcept;

        void resize(Size newReportedSize);

        void drawRect(RectF rect, Color color) override;

        void beginClip(RectF areaRect) override;

        void drawLine(PointF fromPoint, PointF toPoint, Color color) override;

        void drawText(
            PointF originPoint,
            std::string_view text,
            TextScale scale,
            Color color) override;

        void drawTexture(
            const ITexture &texture,
            RectF sourceRect,
            RectF destinationRect,
            Color tintColor) override;

        using ForwardingRenderer::beginTarget;

        void beginTarget(
            IRenderTarget &target,
            std::optional<Rect> regionRect) override;

        void endTarget() override;

        using ForwardingRenderer::drawMesh;

        void drawMesh(
            const IMesh &mesh,
            const Mat4 &modelMatrix,
            const Camera3D &camera,
            const MeshMaterial &material) override;

        void fillLetterbox(Color color);

        [[nodiscard]] bool isTargetBound() const noexcept;

    private:
        void fillIfDrawable(Rect rect, Color color);

        [[nodiscard]] Camera3D getOnWindow(const Camera3D &camera) const;

        Size reportedSize;
        Size canvasSize;
        Fit fit = Fit::Stretched;
        Viewport transformViewport;

        std::size_t bound = 0;
    };

}
