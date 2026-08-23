#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IShader.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/MeshMaterial.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/ShaderSource.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/Viewport.hpp"

namespace antwika::gfx
{

    class ViewportRenderer final : public IRenderer
    {
    public:
        ViewportRenderer(
            IRenderer &innerRenderer,
            Size reportedSize,
            Size canvasSize,
            Fit fit = Fit::Stretched);

        ViewportRenderer(const ViewportRenderer &) = delete;
        ViewportRenderer(ViewportRenderer &&) = delete;

        ViewportRenderer &operator=(const ViewportRenderer &) = delete;
        ViewportRenderer &operator=(ViewportRenderer &&) = delete;

        [[nodiscard]] Viewport getViewport() const noexcept;

        [[nodiscard]] IRenderer &nativeRenderer() noexcept;

        [[nodiscard]] Size getWindowSize() const noexcept;

        void resize(Size newReportedSize);

        void clear(Color color) override;

        void drawRect(RectF rect, Color color) override;

        void beginClip(RectF areaRect) override;

        void endClip() override;

        void drawLine(PointF fromPoint, PointF toPoint, Color color) override;

        void drawText(
            PointF originPoint,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        void updateTexture(
            ITexture &texture, const Bitmap &bitmap) override;

        void drawTexture(
            const ITexture &texture,
            RectF sourceRect,
            RectF destinationRect,
            Color tintColor) override;

        [[nodiscard]] std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) override;

        [[nodiscard]] std::unique_ptr<IShader> createShader(
            const ShaderSource &source) override;

        void setShaderNumber(
            const IShader &shader,
            std::string_view name,
            float value) override;

        void setShaderVector(
            const IShader &shader,
            std::string_view name,
            Vec3 vector) override;

        [[nodiscard]] std::unique_ptr<IRenderTarget> createRenderTarget(
            const RenderTargetSpec &spec) override;

        void beginTarget(IRenderTarget &target) override;

        void beginTargetRegion(
            IRenderTarget &target, Rect regionRect) override;

        void endTarget() override;

        void setShaderMatrix(
            const IShader &shader,
            std::string_view name,
            const Mat4 &matrix) override;

        void setShaderColor(
            const IShader &shader,
            std::string_view name,
            Color valueColor) override;

        using IRenderer::drawMesh;

        void drawMesh(
            const IMesh &mesh,
            const Mat4 &modelMatrix,
            const Camera3D &camera,
            const MeshMaterial &material) override;

        void pushTransform(const Mat4 &matrix) override;

        void popTransform() override;

        void fillLetterbox(Color color);

        [[nodiscard]] bool isTargetBound() const noexcept;

        [[nodiscard]] Bitmap readPixels() override;

        void present() override;

    private:
        void fillIfDrawable(Rect rect, Color color);

        [[nodiscard]] Camera3D getOnWindow(const Camera3D &camera) const;

        IRenderer &innerRenderer;
        Size reportedSize;
        Size canvasSize;
        Fit fit = Fit::Stretched;
        Viewport transformViewport;

        std::size_t bound = 0;
    };

}
