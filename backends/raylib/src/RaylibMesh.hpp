#pragma once

#include <cstddef>

#include <raylib.h>

#include <antwika/gfx/IMesh.hpp>

#include "RaylibResource.hpp"

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibMesh final : public IMesh, public RaylibResource
    {
    public:
        RaylibMesh(RaylibRenderer &ownerRenderer, ::Mesh mesh);

        ~RaylibMesh() override;

        [[nodiscard]] std::size_t getVertexCount() const override;

        [[nodiscard]] std::size_t getTriangleCount() const override;

        [[nodiscard]] const ::Mesh &getRawHandle() const noexcept;

        [[nodiscard]] ::Mesh &writableHandle() noexcept;

    private:
        void unloadHandle() noexcept override;

        ::Mesh mesh;
        std::size_t vertices;
        std::size_t triangles;
    };

}
