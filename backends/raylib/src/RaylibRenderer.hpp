#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GlyphSheetTextures.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IRenderer3D.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::gfx::raylib
{

    class RaylibMaterial;
    class RaylibMesh;
    class RaylibTexture;

    class RaylibRenderer final
        : public IRenderer
        , public IRenderer3D
    {
    public:
        RaylibRenderer();

        RaylibRenderer(const RaylibRenderer &) = delete;
        RaylibRenderer(RaylibRenderer &&) = delete;

        RaylibRenderer &operator=(const RaylibRenderer &) = delete;
        RaylibRenderer &operator=(RaylibRenderer &&) = delete;

        ~RaylibRenderer() override;

        void clear(Color color) override;

        void drawRect(Rect rect, Color color) override;

        void drawLine(Point from, Point to, Color color) override;

        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        [[nodiscard]] IRenderer3D *renderer3d() override;

        [[nodiscard]] std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) override;

        void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) override;

        void present() override;

        void detach();

        void rememberTexture(RaylibTexture &texture);

        void forgetTexture(const RaylibTexture &texture) noexcept;

        void rememberMesh(RaylibMesh &mesh);

        void forgetMesh(const RaylibMesh &mesh) noexcept;

    private:
        void beginIfNeeded();

        bool drawing = false;
        bool attached = true;

        GlyphSheetTextures glyphSheets;

        std::vector<RaylibTexture *> liveTextures;

        std::vector<RaylibMesh *> liveMeshes;

        std::unique_ptr<RaylibMaterial> material;
    };

}
