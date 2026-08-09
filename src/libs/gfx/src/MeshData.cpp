#include "antwika/gfx/MeshData.hpp"

#include <algorithm>
#include <cstddef>

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

    std::size_t MeshData::triangleCount() const
    {
        return indices.size() / 3U;
    }

}
