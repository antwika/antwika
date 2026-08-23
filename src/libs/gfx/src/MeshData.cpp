#include "antwika/gfx/MeshData.hpp"

#include <algorithm>
#include <cstddef>
#include <unordered_map>

#include "antwika/gfx/GfxError.hpp"

namespace antwika::gfx
{

    bool MeshData::isComplete() const
    {
        if (indices.empty() || indices.size() % 3U != 0U)
        {
            return false;
        }

        const auto count = vertices.size();

        return std::ranges::all_of(
            indices,
            [count](std::uint32_t index)
            {
                return static_cast<std::size_t>(index) < count;
            });
    }

    std::size_t MeshData::getTriangleCount() const
    {
        return indices.size() / 3U;
    }

    std::vector<MeshData> getSplitMesh(
        const MeshData &mesh, const std::size_t maxVertices)
    {
        if (!mesh.isComplete())
        {
            throw GfxError(
                "gfx: a mesh cut into pieces must index the vertices "
                "it claims");
        }

        if (maxVertices < 3U)
        {
            throw GfxError(
                "gfx: a piece of a mesh must hold a triangle at least");
        }

        std::vector<MeshData> pieceMeshes;
        std::unordered_map<std::uint32_t, std::uint32_t> movedIndexes;

        pieceMeshes.emplace_back();
        movedIndexes.reserve(maxVertices);

        for (std::size_t triangleStart = 0;
             triangleStart + 2U < mesh.indices.size();
             triangleStart += 3U)
        {
            if (pieceMeshes.back().vertices.size() + 3U > maxVertices)
            {
                pieceMeshes.emplace_back();
                movedIndexes.clear();
            }

            MeshData &pieceData = pieceMeshes.back();

            for (std::size_t corner = 0; corner < 3U; ++corner)
            {
                const std::uint32_t was = mesh.indices[triangleStart + corner];
                const auto foundVertex = movedIndexes.find(was);

                if (foundVertex != movedIndexes.end())
                {
                    pieceData.indices.push_back(foundVertex->second);
                    continue;
                }

                const auto vertexCount =
                    static_cast<std::uint32_t>(pieceData.vertices.size());

                pieceData.vertices.push_back(mesh.vertices[was]);
                pieceData.indices.push_back(vertexCount);
                movedIndexes.emplace(was, vertexCount);
            }
        }

        return pieceMeshes;
    } // GCOVR_EXCL_LINE

}
