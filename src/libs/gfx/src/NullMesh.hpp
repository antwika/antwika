#pragma once

#include <cstddef>

#include "antwika/gfx/IMesh.hpp"

namespace antwika::gfx::detail
{

    class NullMesh final : public IMesh
    {
    public:
        NullMesh(std::size_t vertices, std::size_t triangles);

        NullMesh(const NullMesh &) = delete;
        NullMesh(NullMesh &&) = delete;

        NullMesh &operator=(const NullMesh &) = delete;
        NullMesh &operator=(NullMesh &&) = delete;

        [[nodiscard]] std::size_t vertexCount() const override;

        [[nodiscard]] std::size_t triangleCount() const override;

    private:
        std::size_t vertices;
        std::size_t triangles;
    };

}
