#pragma once

#include <cstddef>

#include "antwika/gfx/IMesh.hpp"

namespace antwika::gfx::fakes
{

    class FakeForeignMesh final : public IMesh
    {
    public:
        [[nodiscard]] std::size_t getVertexCount() const override
        {
            return 3;
        }

        [[nodiscard]] std::size_t getTriangleCount() const override
        {
            return 1;
        }
    };

}
