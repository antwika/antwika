#include "RaylibMesh.hpp"

#include <cstddef>

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibMesh::RaylibMesh(RaylibRenderer &ownerRenderer, ::Mesh mesh)
        : RaylibResource(ownerRenderer),
          mesh(mesh),
          vertices(static_cast<std::size_t>(mesh.vertexCount)),
          triangles(static_cast<std::size_t>(mesh.triangleCount))
    {
    }

    RaylibMesh::~RaylibMesh()
    {
        unload();
    }

    std::size_t RaylibMesh::getVertexCount() const
    {
        return vertices;
    }

    std::size_t RaylibMesh::getTriangleCount() const
    {
        return triangles;
    }

    const ::Mesh &RaylibMesh::getRawHandle() const noexcept
    {
        return mesh;
    }

    ::Mesh &RaylibMesh::writableHandle() noexcept
    {
        return mesh;
    }

    void RaylibMesh::unloadHandle() noexcept
    {
        UnloadMesh(mesh);
    }

}
