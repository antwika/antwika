#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/ClipScope.hpp"
#include "antwika/gfx/ISurfaceRenderer.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/IRenderTarget.hpp"
#include "antwika/gfx/IShader.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"
#include "antwika/gfx/MeshMaterial.hpp"
#include "antwika/gfx/PointF.hpp"
#include "antwika/gfx/RectF.hpp"
#include "antwika/gfx/RenderTargetSpec.hpp"
#include "antwika/gfx/ShaderSource.hpp"
#include "antwika/gfx/TargetScope.hpp"
#include "antwika/gfx/TransformScope.hpp"

namespace antwika::gfx
{

    class IRenderer : public ISurfaceRenderer
    {
    public:
        [[nodiscard]] virtual std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) = 0;

        virtual void updateTexture(
            ITexture &texture, const Bitmap &bitmap) = 0;

        [[nodiscard]] virtual std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) = 0;

        [[nodiscard]] virtual std::unique_ptr<IShader> createShader(
            const ShaderSource &source) = 0;

        [[nodiscard]] virtual std::unique_ptr<IRenderTarget>
        createRenderTarget(const RenderTargetSpec &spec) = 0;

        virtual void beginTarget(IRenderTarget &target) = 0;

        virtual void beginTargetRegion(
            IRenderTarget &target, Rect regionRect) = 0;

        virtual void endTarget() = 0;

        virtual void setShaderNumber(
            const IShader &shader,
            std::string_view name,
            float value) = 0;

        virtual void setShaderVector(
            const IShader &shader,
            std::string_view name,
            Vec3 vector) = 0;

        virtual void setShaderColor(
            const IShader &shader,
            std::string_view name,
            Color valueColor) = 0;

        virtual void setShaderMatrix(
            const IShader &shader,
            std::string_view name,
            const Mat4 &matrix) = 0;

        virtual void drawMesh(
            const IMesh &mesh,
            const Mat4 &modelMatrix,
            const Camera3D &camera,
            const MeshMaterial &material) = 0;

        void drawMesh(
            const IMesh &mesh,
            const Mat4 &modelMatrix,
            const Camera3D &camera,
            const Color tintColor)
        {
            drawMesh(
                mesh,
                modelMatrix,
                camera,
                MeshMaterial{.tintColor = tintColor});
        }

        [[nodiscard]] TargetScope targetScope(IRenderTarget &target)
        {
            beginTarget(target);

            return TargetScope{*this};
        }

        [[nodiscard]] TargetScope targetScope(
            IRenderTarget &target, const Rect regionRect)
        {
            beginTargetRegion(target, regionRect);

            return TargetScope{*this};
        }

        virtual void pushTransform(const Mat4 &transform) = 0;

        [[nodiscard]] TransformScope transformScope(
            const Mat4 &transform)
        {
            pushTransform(transform);

            return TransformScope{*this};
        }

        virtual void popTransform() = 0;

        [[nodiscard]] virtual Bitmap readPixels() = 0;

        virtual void present() = 0;
    };

}
