#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
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

    // Forward-declared rather than included.
    // Their headers name raylib types, and raylib's are global.
    class RaylibMaterial;
    class RaylibMesh;
    class RaylibTexture;

    /**
     * @brief Draws into raylib's one window, in two dimensions and in
     * three.
     *
     * raylib wants drawing bracketed by BeginDrawing/EndDrawing, which
     * IRenderer has no equivalent of. The bracket is opened lazily by the
     * first drawing call and closed by present(), so callers keep the
     * clear/draw/present shape every other backend uses.
     *
     * The 3D half is this same object, as the null backend's is, because
     * both halves draw into one frame through one lazily-opened bracket:
     * two objects would each have to know when the other had opened it.
     */
    class RaylibRenderer final
        : public IRenderer
        , public IRenderer3D
    {
    public:
        /**
         * @brief Construct a renderer with nothing loaded.
         *
         * Declared here and defined in the implementation file, for the
         * reason the destructor is.
         */
        RaylibRenderer();

        RaylibRenderer(const RaylibRenderer &) = delete;
        RaylibRenderer(RaylibRenderer &&) = delete;

        RaylibRenderer &operator=(const RaylibRenderer &) = delete;
        RaylibRenderer &operator=(RaylibRenderer &&) = delete;

        /**
         * @brief Release the material, if detach() has not already.
         *
         * Declared here and defined in the implementation file, because
         * the material is held through a pointer to an incomplete type.
         */
        ~RaylibRenderer() override;

        /**
         * @brief Fill the whole drawable area with one colour.
         * @param color The colour to fill with.
         */
        void clear(Color color) override;

        /**
         * @brief Fill a rectangle with one colour.
         * @param rect The rectangle to fill.
         * @param color The colour to fill it with.
         */
        void drawRect(Rect rect, Color color) override;

        /**
         * @brief Draw a one-pixel-wide line between two points.
         *
         * A line whose ends coincide is drawn as a single pixel rather
         * than passed to DrawLine, which submits a two-vertex GL line
         * primitive that rasterises nothing when both vertices are the
         * same point. IRenderer promises that pixel.
         * @param from One end of the line.
         * @param to The other end.
         * @param color The colour to draw in.
         */
        void drawLine(Point from, Point to, Color color) override;

        /**
         * @brief Draw a line of text in the built-in fixed-cell font.
         *
         * Painted from gfx::glyphRow() as filled rectangles rather than
         * with raylib's own DrawText, even though raylib ships a default
         * font that would make that a one-liner. That font is not
         * fixed-cell, so using it would break the metrics gfx::textSize()
         * promises and make this backend draw a different picture from
         * every other one.
         * @param origin Top-left corner of the first glyph's cell.
         * @param text The characters to draw.
         * @param scale Pixels per glyph pixel.
         * @param color The colour to draw the lit pixels in.
         */
        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        /**
         * @brief Upload a bitmap as a raylib texture.
         * @param bitmap The pixels to upload.
         * @return The new texture, never null.
         * @throws GfxError If the bitmap is not complete, if the window
         * has closed, or if raylib could not hold the pixels.
         */
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        /**
         * @brief Blit part of a texture into part of the window.
         *
         * Declines a texture this renderer did not create, since raylib
         * would otherwise be handed a texture name from a context that
         * has gone.
         * @param texture The pixels to take from.
         * @param source The region of the texture to take.
         * @param destination The region of the window to fill.
         * @param tint Multiplied into the texture's colour and alpha.
         */
        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        /**
         * @brief Offer this renderer's 3D half.
         * @return This renderer, which is also an IRenderer3D.
         */
        [[nodiscard]] IRenderer3D *renderer3d() override;

        /**
         * @brief Upload an indexed triangle list as a raylib mesh.
         *
         * raylib indexes a mesh with 16-bit indices, so a mesh with more
         * vertices than one of those can address is refused rather than
         * silently wrapped around.
         * That is a limit of this backend and not of MeshData, which
         * says 32 bits, so it is reported as the failure it is.
         * @param mesh The geometry to upload.
         * @return The new mesh, never null.
         * @throws GfxError If the data is not complete, if it holds more
         * vertices than a 16-bit index can address, if the window has
         * closed, or if raylib could not hold the geometry.
         */
        [[nodiscard]] std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) override;

        /**
         * @brief Draw a mesh through a camera.
         *
         * Declines a mesh this renderer did not create, exactly as
         * drawTexture() declines a foreign texture and for the same
         * reason.
         * @param mesh The geometry to draw.
         * @param model Takes the mesh's own space to world space.
         * @param camera Takes world space to clip space.
         * @param tint Multiplied into every vertex colour.
         */
        void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) override;

        /**
         * @brief Close the drawing bracket, presenting the frame.
         */
        void present() override;

        /**
         * @brief Close any open bracket before the window goes away.
         *
         * Unloads every live texture and mesh first, and releases the
         * material, because raylib frees each of them through the GL
         * context CloseWindow() destroys.
         */
        void detach();

        /**
         * @brief Start tracking a texture created by this renderer.
         * @param texture The texture, which must call forgetTexture()
         * before it is destroyed.
         */
        void rememberTexture(RaylibTexture &texture);

        /**
         * @brief Stop tracking a texture that is destroying itself.
         * @param texture The texture; one never tracked is ignored.
         */
        void forgetTexture(const RaylibTexture &texture) noexcept;

        /**
         * @brief Start tracking a mesh created by this renderer.
         * @param mesh The mesh, which must call forgetMesh() before it
         * is destroyed.
         */
        void rememberMesh(RaylibMesh &mesh);

        /**
         * @brief Stop tracking a mesh that is destroying itself.
         * @param mesh The mesh; one never tracked is ignored.
         */
        void forgetMesh(const RaylibMesh &mesh) noexcept;

    private:
        void beginIfNeeded();

        bool drawing = false;
        bool attached = true;

        // Not owned: each texture owns itself and deregisters here.
        // Only how detach() reaches them while the context lives.
        std::vector<RaylibTexture *> liveTextures;

        // Meshes are tracked the same way, and for the same reason.
        std::vector<RaylibMesh *> liveMeshes;

        // Loaded on the first mesh drawn, not at construction.
        // A renderer exists before its window's GL context does.
        std::unique_ptr<RaylibMaterial> material;
    };

} // namespace antwika::gfx::raylib
