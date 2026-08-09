#pragma once

#include <cstddef>

namespace antwika::gfx
{

    class IMesh
    {
    public:
        virtual ~IMesh() = default;

        [[nodiscard]] virtual std::size_t vertexCount() const = 0;

        [[nodiscard]] virtual std::size_t triangleCount() const = 0;
    };

}
