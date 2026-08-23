#pragma once

#include <cstddef>

namespace antwika::gfx
{

    class IMesh
    {
    public:
        virtual ~IMesh() = default;

        [[nodiscard]] virtual std::size_t getVertexCount() const = 0;

        [[nodiscard]] virtual std::size_t getTriangleCount() const = 0;
    };

}
