#include "NullMesh.hpp"

#include <cstddef>

namespace antwika::gfx::detail
{

    NullMesh::NullMesh(std::size_t vertices, std::size_t triangles)
        : vertices(vertices)
        , triangles(triangles)
    {
    }

    std::size_t NullMesh::getVertexCount() const
    {
        return vertices;
    }

    std::size_t NullMesh::getTriangleCount() const
    {
        return triangles;
    }

}
