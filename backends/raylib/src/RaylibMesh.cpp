#include "RaylibMesh.hpp"

#include <cstddef>

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibMesh::RaylibMesh(RaylibRenderer &owner, ::Mesh mesh)
        : owner(&owner),
          mesh(mesh),
          vertices(static_cast<std::size_t>(mesh.vertexCount)),
          triangles(static_cast<std::size_t>(mesh.triangleCount))
    {
        owner.rememberMesh(*this);
    }

    RaylibMesh::~RaylibMesh()
    {
        if (owner != nullptr)
        {
            owner->forgetMesh(*this);
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

    bool RaylibMesh::belongsTo(
        const RaylibRenderer &candidate) const noexcept
    {
        return owner == &candidate;
    }

    const ::Mesh &RaylibMesh::raw() const noexcept
    {
        return mesh;
    }

    bool RaylibMesh::isLoaded() const noexcept
    {
        return loaded;
    }

    void RaylibMesh::forgetRenderer() noexcept
    {
        owner = nullptr;
        loaded = false;
    }

} // namespace antwika::gfx::raylib
