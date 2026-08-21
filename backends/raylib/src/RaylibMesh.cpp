#include "RaylibMesh.hpp"

#include <cstddef>

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibMesh::RaylibMesh(RaylibRenderer &ownerRenderer, ::Mesh mesh)
        : owner(&ownerRenderer),
          mesh(mesh),
          vertices(static_cast<std::size_t>(mesh.vertexCount)),
          triangles(static_cast<std::size_t>(mesh.triangleCount))
    {
        ownerRenderer.trackMesh(*this);
    }

    RaylibMesh::~RaylibMesh()
    {
        if (owner != nullptr)
        {
            owner->untrackMesh(*this);
        }

        if (loaded)
        {
            UnloadMesh(mesh);
        }
    }

    std::size_t RaylibMesh::vertexCount() const
    {
        return vertices;
    }

    std::size_t RaylibMesh::triangleCount() const
    {
        return triangles;
    }

    bool RaylibMesh::isOwnedBy(
        const RaylibRenderer &candidateRenderer) const noexcept
    {
        return owner == &candidateRenderer;
    }

    const ::Mesh &RaylibMesh::raw() const noexcept
    {
        return mesh;
    }

    bool RaylibMesh::isLoaded() const noexcept
    {
        return loaded;
    }

    void RaylibMesh::untrackRenderer() noexcept
    {
        owner = nullptr;
        loaded = false;
    }

}
