#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IShader.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"
#include "antwika/gfx/MeshMaterial.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/RenderTargetSpec.hpp"
#include "antwika/gfx/ShaderSource.hpp"

namespace antwika::gfx
{

    class ForwardingRenderer : public IRenderer
    {
    public:
        explicit ForwardingRenderer(IRenderer &innerRenderer);

        ForwardingRenderer(const ForwardingRenderer &) = delete;
        ForwardingRenderer(ForwardingRenderer &&) = delete;

        ForwardingRenderer &operator=(const ForwardingRenderer &) = delete;
        ForwardingRenderer &operator=(ForwardingRenderer &&) = delete;

        void clear(Color color) override;

        void drawRect(RectF rect, Color color) override;

        void beginClip(RectF areaRect) override;

        void endClip() override;

        void drawLine(PointF fromPoint, PointF toPoint, Color color) override;

        void drawText(
            PointF originPoint,
            std::string_view text,
            TextScale scale,
            Color color) override;

        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        void updateTexture(
            ITexture &texture, const Bitmap &bitmap) override;

        void updateMesh(IMesh &mesh, const MeshData &data) override;

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

        using IRenderer::beginTarget;

        void beginTarget(
            IRenderTarget &target,
            std::optional<Rect> regionRect) override;

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

        void pushTransform(const Mat4 &transform) override;

        void popTransform() override;

        [[nodiscard]] Bitmap readPixels() override;

        void present() override;

    protected:
        [[nodiscard]] IRenderer &innerRenderer() noexcept;

    private:
        IRenderer &wrappedRenderer;
    };

}
