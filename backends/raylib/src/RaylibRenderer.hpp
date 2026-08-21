#pragma once

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GlyphAtlasTextures.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/ShaderSource.hpp>

namespace antwika::gfx::raylib
{

    class RaylibMaterial;
    class RaylibMesh;
    class RaylibRenderTarget;
    class RaylibShader;
    class RaylibTexture;

    class RaylibRenderer final : public IRenderer
    {
    public:
        RaylibRenderer();

        RaylibRenderer(const RaylibRenderer &) = delete;
        RaylibRenderer(RaylibRenderer &&) = delete;

        RaylibRenderer &operator=(const RaylibRenderer &) = delete;
        RaylibRenderer &operator=(RaylibRenderer &&) = delete;

        ~RaylibRenderer() override;

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

        void pushTransform(const Mat4 &transform) override;

        void popTransform() override;

        [[nodiscard]] Bitmap readPixels() override;

        void present() override;

        void detach();

        void trackTexture(RaylibTexture &texture);

        void untrackTexture(const RaylibTexture &texture) noexcept;

        void trackMesh(RaylibMesh &mesh);

        void untrackMesh(const RaylibMesh &mesh) noexcept;

        void trackShader(RaylibShader &shader);

        void untrackShader(const RaylibShader &shader) noexcept;

        void trackTarget(RaylibRenderTarget &target);

        void untrackTarget(const RaylibRenderTarget &target) noexcept;

    private:
        void ensureDrawing();

        std::vector<RectF> clipRects;

        [[nodiscard]] const ::Texture2D *ownTextureOf(
            const ITexture *texture) const noexcept;

        [[nodiscard]] const ::Shader *ownShaderOf(
            const IShader *shader) const noexcept;

        [[nodiscard]] int uniformLocationOf(
            const IShader &shader,
            const ::Shader &nativeShader,
            std::string_view name);

        void setShaderValue(
            const IShader &shader,
            std::string_view name,
            const void *value,
            int kind);

        bool drawing = false;
        bool attached = true;

        std::size_t pushedCount = 0;

        GlyphAtlasTextures glyphAtlases;

        std::vector<RaylibTexture *> liveTextures;

        std::vector<RaylibMesh *> liveMeshes;

        std::vector<RaylibShader *> liveShaders;

        struct NameHash final
        {
            using is_transparent = void;

            [[nodiscard]] std::size_t operator()(
                std::string_view name) const noexcept
            {
                return std::hash<std::string_view>{}(name);
            }
        };

        std::unordered_map<
            const IShader *,
            std::unordered_map<
                std::string,
                int,
                NameHash,
                std::equal_to<>>>
            uniformLocations;

        std::vector<RaylibRenderTarget *> liveTargets;

        void applyRegionViewport();

        RaylibRenderTarget *inTarget = nullptr;
        std::optional<Rect> inRegionRect;

        std::unique_ptr<RaylibMaterial> material;
    };

}
