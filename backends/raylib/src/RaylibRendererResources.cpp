#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>

#include "RaylibRenderer.hpp"
#include "RaylibMesh.hpp"
#include "RaylibRenderTarget.hpp"
#include "RaylibShader.hpp"
#include "RaylibTexture.hpp"

namespace antwika::gfx::raylib
{

    namespace
    {
        constexpr std::size_t kMaxMeshVertices = 65536;

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

        [[nodiscard]] ::Image getLentImage(const Bitmap &bitmap)
        {
            return ::Image{
                .data = const_cast<std::uint8_t *>(
                    bitmap.pixels.data()),
                .width = static_cast<int>(bitmap.size.width),
                .height = static_cast<int>(bitmap.size.height),
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        }
    }

    std::unique_ptr<ITexture> RaylibRenderer::createTexture(
        const Bitmap &bitmap)
    {
        if (!bitmap.isValid())
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

        const ::Image source = getLentImage(bitmap);

        const ::Texture2D texture = LoadTextureFromImage(source);

        if (!IsTextureValid(texture))
        {
            throw GfxError("gfx.raylib: could not create a texture");
        }

        SetTextureFilter(texture, TEXTURE_FILTER_POINT);

        return std::make_unique<RaylibTexture>(
            *this, texture, bitmap.size);
    }

    void RaylibRenderer::updateTexture(
        ITexture &texture, const Bitmap &bitmap)
    {
        auto *mine = dynamic_cast<RaylibTexture *>(&texture);

        if (mine == nullptr || !mine->isOwnedBy(*this) || !mine->isLoaded())
        {
            sayRefused("a texture this renderer does not hold was updated");

            return;
        }

        if (!bitmap.isValid() || mine->getSize() != bitmap.size)
        {
            sayRefused("a texture was updated from a picture of "
                       "another size");

            return;
        }

        if (!attached)
        {
            return;
        }

        ::UpdateTexture(mine->getRawHandle(), bitmap.pixels.data());
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

        ::Mesh nativeMesh{};
        nativeMesh.vertexCount = static_cast<int>(vertexCount);
        nativeMesh.triangleCount = static_cast<int>(mesh.getTriangleCount());

        try
        {
            nativeMesh.vertices = allocateFor<float>(vertexCount * 3);
            nativeMesh.normals = allocateFor<float>(vertexCount * 3);
            nativeMesh.texcoords = allocateFor<float>(vertexCount * 2);
            nativeMesh.colors = allocateFor<unsigned char>(vertexCount * 4);
            nativeMesh.indices = allocateFor<unsigned short>(indexCount);
        }
        catch (...)
        {
            freeArrays(nativeMesh);
            throw;
        }

        for (std::size_t vertex = 0; vertex < vertexCount; ++vertex)
        {
            const Vertex3D &sourceVertex = mesh.vertices[vertex];

            nativeMesh.vertices[(vertex * 3) + 0] = sourceVertex.position.x;
            nativeMesh.vertices[(vertex * 3) + 1] = sourceVertex.position.y;
            nativeMesh.vertices[(vertex * 3) + 2] = sourceVertex.position.z;

            nativeMesh.normals[(vertex * 3) + 0] = sourceVertex.normal.x;
            nativeMesh.normals[(vertex * 3) + 1] = sourceVertex.normal.y;
            nativeMesh.normals[(vertex * 3) + 2] = sourceVertex.normal.z;

            nativeMesh.texcoords[(vertex * 2) + 0] =
                sourceVertex.texCoordinate.x;
            nativeMesh.texcoords[(vertex * 2) + 1] =
                sourceVertex.texCoordinate.y;

            nativeMesh.colors[(vertex * 4) + 0] = sourceVertex.color.red;
            nativeMesh.colors[(vertex * 4) + 1] = sourceVertex.color.green;
            nativeMesh.colors[(vertex * 4) + 2] = sourceVertex.color.blue;
            nativeMesh.colors[(vertex * 4) + 3] = sourceVertex.color.alpha;
        }

        for (std::size_t index = 0; index < indexCount; ++index)
        {
            nativeMesh.indices[index] =
                static_cast<unsigned short>(mesh.indices[index]);
        }

        UploadMesh(&nativeMesh, false);

        if (nativeMesh.vboId == nullptr)
        {
            UnloadMesh(nativeMesh);

            throw GfxError("gfx.raylib: could not upload the geometry");
        }

        return std::make_unique<RaylibMesh>(*this, nativeMesh);
    }

    void RaylibRenderer::updateMesh(IMesh &mesh, const MeshData &data)
    {
        auto *mine = dynamic_cast<RaylibMesh *>(&mesh);

        if (mine == nullptr || !mine->isOwnedBy(*this) || !mine->isLoaded())
        {
            sayRefused("a mesh this renderer does not hold was updated");

            return;
        }

        if (!data.isComplete()
            || mine->getVertexCount() != data.vertices.size()
            || mine->getTriangleCount() != data.getTriangleCount())
        {
            sayRefused(
                "a mesh was updated from geometry of another shape");

            return;
        }

        if (!attached)
        {
            return;
        }

        auto &nativeMesh = mine->writableHandle();
        const auto vertexCount = data.vertices.size();

        for (std::size_t vertex = 0; vertex < vertexCount; ++vertex)
        {
            const Vertex3D &sourceVertex = data.vertices[vertex];

            nativeMesh.vertices[(vertex * 3) + 0] = sourceVertex.position.x;
            nativeMesh.vertices[(vertex * 3) + 1] = sourceVertex.position.y;
            nativeMesh.vertices[(vertex * 3) + 2] = sourceVertex.position.z;

            nativeMesh.normals[(vertex * 3) + 0] = sourceVertex.normal.x;
            nativeMesh.normals[(vertex * 3) + 1] = sourceVertex.normal.y;
            nativeMesh.normals[(vertex * 3) + 2] = sourceVertex.normal.z;

            nativeMesh.texcoords[(vertex * 2) + 0] =
                sourceVertex.texCoordinate.x;
            nativeMesh.texcoords[(vertex * 2) + 1] =
                sourceVertex.texCoordinate.y;

            nativeMesh.colors[(vertex * 4) + 0] = sourceVertex.color.red;
            nativeMesh.colors[(vertex * 4) + 1] = sourceVertex.color.green;
            nativeMesh.colors[(vertex * 4) + 2] = sourceVertex.color.blue;
            nativeMesh.colors[(vertex * 4) + 3] = sourceVertex.color.alpha;
        }

        UpdateMeshBuffer(
            nativeMesh,
            0,
            nativeMesh.vertices,
            static_cast<int>(vertexCount * 3 * sizeof(float)),
            0);
        UpdateMeshBuffer(
            nativeMesh,
            1,
            nativeMesh.texcoords,
            static_cast<int>(vertexCount * 2 * sizeof(float)),
            0);
        UpdateMeshBuffer(
            nativeMesh,
            2,
            nativeMesh.normals,
            static_cast<int>(vertexCount * 3 * sizeof(float)),
            0);
        UpdateMeshBuffer(
            nativeMesh,
            3,
            nativeMesh.colors,
            static_cast<int>(vertexCount * 4 * sizeof(unsigned char)),
            0);
    }

    std::unique_ptr<IShader> RaylibRenderer::createShader(
        const ShaderSource &source)
    {
        if (!source.isComplete())
        {
            throw GfxError(
                "gfx.raylib: shader source is missing a stage");
        }

        if (!attached)
        {
            throw GfxError(
                "gfx.raylib: the window this renderer drew into has "
                "closed");
        }

        ::Shader nativeShader = LoadShaderFromMemory(
            source.vertex.c_str(), source.fragment.c_str());

        if (!IsShaderValid(nativeShader))
        {
            UnloadShader(nativeShader);

            throw GfxError(
                "gfx.raylib: could not compile and link the shader");
        }

        nativeShader.locs[SHADER_LOC_MAP_ROUGHNESS] =
            GetShaderLocation(nativeShader, "texture3");
        nativeShader.locs[SHADER_LOC_MAP_OCCLUSION] =
            GetShaderLocation(nativeShader, "texture4");

        return std::make_unique<RaylibShader>(*this, nativeShader);
    }

    std::unique_ptr<IRenderTarget> RaylibRenderer::createRenderTarget(
        const RenderTargetSpec &spec)
    {
        if (spec.size.width == 0 || spec.size.height == 0)
        {
            throw GfxError(
                "gfx.raylib: a render target needs a size");
        }

        if (!attached)
        {
            throw GfxError(
                "gfx.raylib: the window this renderer drew into has "
                "closed");
        }

        return std::make_unique<RaylibRenderTarget>(*this, spec);
    }

    void RaylibRenderer::trackTarget(RaylibRenderTarget &target)
    {
        liveTargets.push_back(&target);
    }

    void RaylibRenderer::untrackTarget(
        const RaylibRenderTarget &target) noexcept
    {
        if (inTarget == &target)
        {
            inTarget = nullptr;
        }

        for (auto liveTarget = liveTargets.begin();
             liveTarget != liveTargets.end();
             ++liveTarget)
        {
            if (*liveTarget == &target)
            {
                liveTargets.erase(liveTarget);
                return;
            }
        }
    }

    void RaylibRenderer::trackTexture(RaylibTexture &texture)
    {
        liveTextures.push_back(&texture);
    }

    void RaylibRenderer::untrackTexture(
        const RaylibTexture &texture) noexcept
    {
        std::erase(liveTextures, &texture);
    }

    void RaylibRenderer::trackMesh(RaylibMesh &mesh)
    {
        liveMeshes.push_back(&mesh);
    }

    void RaylibRenderer::untrackMesh(const RaylibMesh &mesh) noexcept
    {
        std::erase(liveMeshes, &mesh);
    }

    void RaylibRenderer::trackShader(RaylibShader &shader)
    {
        liveShaders.push_back(&shader);
    }

    void RaylibRenderer::untrackShader(
        const RaylibShader &shader) noexcept
    {
        std::erase(liveShaders, &shader);
        uniformLocations.erase(&shader);
    }

    const ::Texture2D *RaylibRenderer::ownTextureOf(
        const ITexture *texture) const noexcept
    {
        const auto *mine = dynamic_cast<const RaylibTexture *>(texture);

        if (mine == nullptr || !mine->isOwnedBy(*this)
            || !mine->isLoaded())
        {
            if (texture != nullptr)
            {
                sayRefused(
                    "a texture this renderer does not hold was bound");
            }

            return nullptr;
        }

        return &mine->getRawHandle();
    }

    const ::Shader *RaylibRenderer::ownShaderOf(
        const IShader *shader) const noexcept
    {
        const auto *mine = dynamic_cast<const RaylibShader *>(shader);

        if (mine == nullptr || !mine->isOwnedBy(*this)
            || !mine->isLoaded())
        {
            return nullptr;
        }

        return &mine->getRawHandle();
    }

}
