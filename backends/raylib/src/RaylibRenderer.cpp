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

        constexpr std::size_t kMaxMeshVertices = 65536;

        constexpr std::size_t kMaxTransformDepth = 32;

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

        void freeArrays(const ::Mesh &mesh) noexcept
        {
            std::free(mesh.vertices);
            std::free(mesh.normals);
            std::free(mesh.texcoords);
            std::free(mesh.colors);
            std::free(mesh.indices);
        }
    }

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

    void RaylibRenderer::drawRect(RectF rect, Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        DrawRectangleRec(
            ::Rectangle{
                .x = rect.origin.x,
                .y = rect.origin.y,
                .width = rect.size.width,
                .height = rect.size.height},
            toRaylib(color));
    }

    void RaylibRenderer::drawLine(PointF from, PointF to, Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        const auto raylibColor = toRaylib(color);

        const ::Vector2 start{.x = from.x, .y = from.y};
        const ::Vector2 end{.x = to.x, .y = to.y};

        if (from == to)
        {
            DrawPixelV(start, raylibColor);
            return;
        }

        DrawLineV(start, end, raylibColor);
    }

    void RaylibRenderer::drawText(
        PointF origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        glyphSheets.draw(*this, origin, text, scale, color);
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

        SetTextureFilter(texture, TEXTURE_FILTER_POINT);

        return std::make_unique<RaylibTexture>(
            *this, texture, bitmap.size);
    }

    void RaylibRenderer::drawTexture(
        const ITexture &texture,
        RectF source,
        RectF destination,
        Color tint)
    {
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
            .x = source.origin.x,
            .y = source.origin.y,
            .width = source.size.width,
            .height = source.size.height};

        const ::Rectangle to{
            .x = destination.origin.x,
            .y = destination.origin.y,
            .width = destination.size.width,
            .height = destination.size.height};

        DrawTexturePro(
            mine->raw(),
            from,
            to,
            ::Vector2{.x = 0.0F, .y = 0.0F},
            0.0F,
            toRaylib(tint));
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
            raw.indices[index] =
                static_cast<unsigned short>(mesh.indices[index]);
        }

        UploadMesh(&raw, false);

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

        rlDrawRenderBatchActive();

        const ::Matrix wasProjection = rlGetMatrixProjection();
        const ::Matrix wasModelview = rlGetMatrixModelview();

        rlSetMatrixProjection(toRaylib(camera.projectionMatrix()));
        rlSetMatrixModelview(toRaylib(camera.view()));
        rlEnableDepthTest();

        DrawMesh(mine->raw(), material->raw(), toRaylib(model));

        rlDisableDepthTest();
        rlSetMatrixModelview(wasModelview);
        rlSetMatrixProjection(wasProjection);
    }

    void RaylibRenderer::pushTransform(const Mat4 &transform)
    {
        if (pushed == kMaxTransformDepth)
        {
            throw GfxError(
                "gfx.raylib: the transform stack is full");
        }

        beginIfNeeded();

        ++pushed;

        if (!drawing)
        {
            return;
        }

        rlPushMatrix();
        rlMultMatrixf(&transform[0][0]);
    }

    void RaylibRenderer::popTransform()
    {
        if (pushed == 0)
        {
            throw GfxError("gfx.raylib: no transform is pushed");
        }

        --pushed;

        if (!drawing)
        {
            return;
        }

        rlPopMatrix();
    }

    void RaylibRenderer::present()
    {
        if (!drawing)
        {
            return;
        }

        while (pushed > 0)
        {
            rlPopMatrix();
            --pushed;
        }

        EndDrawing();
        drawing = false;

        antwika::raylib::advanceFrame();
    }

    void RaylibRenderer::detach()
    {
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

}
