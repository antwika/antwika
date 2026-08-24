#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/log/ILogger.hpp>

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
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/ShaderSource.hpp"

namespace antwika::gfx::detail
{

    using antwika::log::ILogger;

    class NullRenderer final : public IRenderer
    {
    public:
        explicit NullRenderer(ILogger &logger);

        NullRenderer(const NullRenderer &) = delete;
        NullRenderer(NullRenderer &&) = delete;

        NullRenderer &operator=(const NullRenderer &) = delete;
        NullRenderer &operator=(NullRenderer &&) = delete;

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

        void pushTransform(const Mat4 &transform) override;

        void popTransform() override;

        [[nodiscard]] Bitmap readPixels() override;

        void present() override;

    private:
        ILogger &logger;
        std::size_t pushedCount = 0;
    };

}
