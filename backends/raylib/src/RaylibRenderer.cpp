#include "RaylibRenderer.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Blit.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/TextRaster.hpp>

#include "RaylibFrame.hpp"
#include "RaylibMaterial.hpp"
#include "RaylibMesh.hpp"
#include "RaylibTexture.hpp"

namespace antwika::gfx::raylib
{

    namespace
    {
        ::Color toRaylib(Color color)
        {
            return ::Color{
                .r = color.red,
                .g = color.green,
                .b = color.blue,
                .a = color.alpha};
        }

        /**
         * @brief Rename a GLM matrix into raylib's element names.
         *
         * GLM addresses an element as [column][row]; raylib names one
         * mN, where N is the column times four plus the row.
         * Both hold a column-major matrix applied to a column vector on
         * its right, so this is a rename and not a transpose -- and
         * getting that backwards is precisely what silently mirrors a
         * scene, which is why it is spelled out element by element
         * rather than memcpy'd.
         */
        ::Matrix toRaylib(const Mat4 &matrix)
        {
            return ::Matrix{
                .m0 = matrix[0][0],
                .m4 = matrix[1][0],
                .m8 = matrix[2][0],
                .m12 = matrix[3][0],
                .m1 = matrix[0][1],
                .m5 = matrix[1][1],
                .m9 = matrix[2][1],
                .m13 = matrix[3][1],
                .m2 = matrix[0][2],
                .m6 = matrix[1][2],
                .m10 = matrix[2][2],
                .m14 = matrix[3][2],
                .m3 = matrix[0][3],
                .m7 = matrix[1][3],
                .m11 = matrix[2][3],
                .m15 = matrix[3][3]};
        }

        /// How many vertices a 16-bit mesh index can address.
        constexpr std::size_t kMaxMeshVertices = 65536;

        /**
         * @brief Obtain one of a mesh's vertex arrays.
         *
         * std::malloc rather than new[] or a container, because
         * UnloadMesh() frees these with RL_FREE -- std::free in every
         * configuration this project builds raylib in -- and freeing
         * memory the other allocator owns is undefined.
         * @param count How many elements are wanted.
         * @return The array, never null.
         * @throws GfxError If the memory was not available.
         */
        template <typename T>
        T *allocateFor(std::size_t count)
        {
            auto *values =
                static_cast<T *>(std::malloc(count * sizeof(T)));

            if (values == nullptr)
            {
                throw GfxError(
                    "gfx.raylib: could not hold the geometry");
            }

            return values;
        }

        /**
         * @brief Give back arrays that never reached raylib.
         * @param mesh The mesh whose arrays to free; each may be null.
         */
        void freeArrays(const ::Mesh &mesh) noexcept
        {
            std::free(mesh.vertices);
            std::free(mesh.normals);
            std::free(mesh.texcoords);
            std::free(mesh.colors);
            std::free(mesh.indices);
        }
    } // namespace

    RaylibRenderer::RaylibRenderer() = default;

    RaylibRenderer::~RaylibRenderer() = default;

    void RaylibRenderer::clear(Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        ClearBackground(toRaylib(color));
    }

    void RaylibRenderer::drawRect(Rect rect, Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        DrawRectangle(
            rect.origin.x,
            rect.origin.y,
            static_cast<int>(rect.size.width),
            static_cast<int>(rect.size.height),
            toRaylib(color));
    }

    void RaylibRenderer::drawLine(Point from, Point to, Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        const auto raylibColor = toRaylib(color);

        // A GL line between two identical vertices covers no pixel.
        // IRenderer promises that pixel, so draw it directly.
        if (from == to)
        {
            DrawPixel(from.x, from.y, raylibColor);
            return;
        }

        DrawLine(from.x, from.y, to.x, to.y, raylibColor);
    }

    void RaylibRenderer::drawText(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        if (scale == 0)
        {
            return;
        }

        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        // Which pixel is inked is gfx's answer, not this backend's.
        // What colour its coverage leaves it is gfx's answer too.
        // Every backend draws the same glyphs, in the same colours.
        forEachGlyphPixel(
            glyphCells,
            origin,
            text,
            scale,
            color,
            [](Rect pixel, Color pixelColor) {
                DrawRectangle(
                    pixel.origin.x,
                    pixel.origin.y,
                    static_cast<int>(pixel.size.width),
                    static_cast<int>(pixel.size.height),
                    toRaylib(pixelColor));
            });
    }

    std::unique_ptr<ITexture> RaylibRenderer::createTexture(
        const Bitmap &bitmap)
    {
        if (!bitmap.isComplete())
        {
            throw GfxError(
                "gfx.raylib: bitmap does not hold the pixels it claims");
        }

        if (!attached)
        {
            throw GfxError(
                "gfx.raylib: the window this renderer drew into has "
                "closed");
        }

        // Copied because raylib's Image::data is a non-const void *.
        // LoadTextureFromImage only reads it, and never frees it.
        std::vector<std::uint8_t> pixels = bitmap.pixels;

        const ::Image source{
            .data = pixels.data(),
            .width = static_cast<int>(bitmap.size.width),
            .height = static_cast<int>(bitmap.size.height),
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

        const ::Texture2D texture = LoadTextureFromImage(source);

        if (!IsTextureValid(texture))
        {
            throw GfxError("gfx.raylib: could not create a texture");
        }

        // Already what rlLoadTexture leaves a new texture at.
        // Said out loud anyway, because IRenderer now promises it.
        // A default nobody wrote down is one a version bump may change.
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);

        return std::make_unique<RaylibTexture>(
            *this, texture, bitmap.size);
    }

    void RaylibRenderer::drawTexture(
        const ITexture &texture, Rect source, Rect destination, Color tint)
    {
        // ITexture exposes no native handle, on purpose.
        // Reaching raylib's means asking whether this is even ours.
        const auto *mine = dynamic_cast<const RaylibTexture *>(&texture);

        if (mine == nullptr || !mine->belongsTo(*this) || !mine->isLoaded())
        {
            return;
        }

        if (!blitIsDrawable(mine->size(), source, destination))
        {
            return;
        }

        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        const ::Rectangle from{
            .x = static_cast<float>(source.origin.x),
            .y = static_cast<float>(source.origin.y),
            .width = static_cast<float>(source.size.width),
            .height = static_cast<float>(source.size.height)};

        const ::Rectangle to{
            .x = static_cast<float>(destination.origin.x),
            .y = static_cast<float>(destination.origin.y),
            .width = static_cast<float>(destination.size.width),
            .height = static_cast<float>(destination.size.height)};

        DrawTexturePro(
            mine->raw(),
            from,
            to,
            ::Vector2{.x = 0.0F, .y = 0.0F},
            0.0F,
            toRaylib(tint));
    }

    IRenderer3D *RaylibRenderer::renderer3d()
    {
        return this;
    }

    std::unique_ptr<IMesh> RaylibRenderer::createMesh(const MeshData &mesh)
    {
        if (!mesh.isComplete())
        {
            throw GfxError(
                "gfx.raylib: mesh does not index the vertices it claims");
        }

        if (mesh.vertices.size() > kMaxMeshVertices)
        {
            throw GfxError(
                "gfx.raylib: mesh holds more vertices than a 16-bit "
                "index can address");
        }

        if (!attached)
        {
            throw GfxError(
                "gfx.raylib: the window this renderer drew into has "
                "closed");
        }

        const auto vertexCount = mesh.vertices.size();
        const auto indexCount = mesh.indices.size();

        ::Mesh raw{};
        raw.vertexCount = static_cast<int>(vertexCount);
        raw.triangleCount = static_cast<int>(mesh.triangleCount());

        try
        {
            raw.vertices = allocateFor<float>(vertexCount * 3);
            raw.normals = allocateFor<float>(vertexCount * 3);
            raw.texcoords = allocateFor<float>(vertexCount * 2);
            raw.colors = allocateFor<unsigned char>(vertexCount * 4);
            raw.indices = allocateFor<unsigned short>(indexCount);
        }
        catch (...)
        {
            // Nothing has reached raylib yet.
            // So this is a plain free of what was already obtained.
            freeArrays(raw);
            throw;
        }

        for (std::size_t vertex = 0; vertex < vertexCount; ++vertex)
        {
            const Vertex3D &source = mesh.vertices[vertex];

            raw.vertices[(vertex * 3) + 0] = source.position.x;
            raw.vertices[(vertex * 3) + 1] = source.position.y;
            raw.vertices[(vertex * 3) + 2] = source.position.z;

            raw.normals[(vertex * 3) + 0] = source.normal.x;
            raw.normals[(vertex * 3) + 1] = source.normal.y;
            raw.normals[(vertex * 3) + 2] = source.normal.z;

            raw.texcoords[(vertex * 2) + 0] = source.texCoord.x;
            raw.texcoords[(vertex * 2) + 1] = source.texCoord.y;

            raw.colors[(vertex * 4) + 0] = source.color.red;
            raw.colors[(vertex * 4) + 1] = source.color.green;
            raw.colors[(vertex * 4) + 2] = source.color.blue;
            raw.colors[(vertex * 4) + 3] = source.color.alpha;
        }

        for (std::size_t index = 0; index < indexCount; ++index)
        {
            // Narrowed rather than truncated.
            // The vertex count was checked against a 16-bit reach.
            // isComplete() has already refused an index past it.
            raw.indices[index] =
                static_cast<unsigned short>(mesh.indices[index]);
        }

        UploadMesh(&raw, false);

        // The one thing UploadMesh always leaves behind on success.
        // A vertex array object is optional; the buffer ids are not.
        if (raw.vboId == nullptr)
        {
            UnloadMesh(raw);

            throw GfxError("gfx.raylib: could not upload the geometry");
        }

        return std::make_unique<RaylibMesh>(*this, raw);
    }

    void RaylibRenderer::drawMesh(
        const IMesh &mesh,
        const Mat4 &model,
        const Camera3D &camera,
        Color tint)
    {
        // IMesh exposes no native handle, on purpose.
        // Reaching raylib's means asking whether this is even ours.
        const auto *mine = dynamic_cast<const RaylibMesh *>(&mesh);

        if (mine == nullptr || !mine->belongsTo(*this) || !mine->isLoaded())
        {
            return;
        }

        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        if (material == nullptr)
        {
            material = std::make_unique<RaylibMaterial>();
        }

        material->setTint(tint);

        // The matrices go in through rlgl, deliberately.
        // Handing raylib a ::Camera3D to BeginMode3D() would not do.
        // That struct says a field of view and no clip planes.
        // raylib picks the planes itself, from constants of its own.
        // A gfx::Camera3D's near and far planes would be discarded.
        // So would an orthographic one's extents.
        // Nothing would fail; the scene would simply be wrong.
        // Setting these two matrices is all BeginMode3D does anyway.
        // This is raylib's own path, walked with our own numbers.
        rlDrawRenderBatchActive();

        const ::Matrix wasProjection = rlGetMatrixProjection();
        const ::Matrix wasModelview = rlGetMatrixModelview();

        rlSetMatrixProjection(toRaylib(camera.projectionMatrix()));
        rlSetMatrixModelview(toRaylib(camera.view()));
        rlEnableDepthTest();

        // DrawMesh reads the modelview as the view matrix.
        // It folds the transform it is handed into that.
        // So the shader is given projection * view * model.
        // Which is the composition Math3D.hpp describes.
        DrawMesh(mine->raw(), material->raw(), toRaylib(model));

        // Put the 2D frame back exactly as it was found.
        // Anything drawn after this must land where IRenderer says.
        rlDisableDepthTest();
        rlSetMatrixModelview(wasModelview);
        rlSetMatrixProjection(wasProjection);
    }

    void RaylibRenderer::present()
    {
        if (!drawing)
        {
            return;
        }

        EndDrawing();
        drawing = false;

        // The one place raylib's input state moves.
        // The input backend reads this to tell frames apart.
        antwika::raylib::advanceFrame();
    }

    void RaylibRenderer::detach()
    {
        // Before CloseWindow takes the GL context these need.
        // Each is left valid but empty, since one may outlive us.
        for (RaylibTexture *texture : liveTextures)
        {
            UnloadTexture(texture->raw());
            texture->forgetRenderer();
        }

        liveTextures.clear();

        for (RaylibMesh *mesh : liveMeshes)
        {
            UnloadMesh(mesh->raw());
            mesh->forgetRenderer();
        }

        liveMeshes.clear();

        // The material names the default shader and texture.
        // The same context owns both, so it goes at the same time.
        material.reset();

        present();
        attached = false;
    }

    void RaylibRenderer::rememberTexture(RaylibTexture &texture)
    {
        liveTextures.push_back(&texture);
    }

    void RaylibRenderer::forgetTexture(
        const RaylibTexture &texture) noexcept
    {
        std::erase(liveTextures, &texture);
    }

    void RaylibRenderer::rememberMesh(RaylibMesh &mesh)
    {
        liveMeshes.push_back(&mesh);
    }

    void RaylibRenderer::forgetMesh(const RaylibMesh &mesh) noexcept
    {
        std::erase(liveMeshes, &mesh);
    }

    void RaylibRenderer::beginIfNeeded()
    {
        if (drawing || !attached)
        {
            return;
        }

        BeginDrawing();
        drawing = true;
    }

} // namespace antwika::gfx::raylib
