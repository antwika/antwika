#pragma once

#include <cstddef>

#include <raylib.h>

#include <antwika/gfx/IMesh.hpp>

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibMesh final : public IMesh
    {
    public:
        RaylibMesh(RaylibRenderer &ownerRenderer, ::Mesh mesh);

        RaylibMesh(const RaylibMesh &) = delete;
        RaylibMesh(RaylibMesh &&) = delete;

        RaylibMesh &operator=(const RaylibMesh &) = delete;
        RaylibMesh &operator=(RaylibMesh &&) = delete;

        ~RaylibMesh() override;

        [[nodiscard]] std::size_t getVertexCount() const override;

        [[nodiscard]] std::size_t getTriangleCount() const override;

        [[nodiscard]] bool isOwnedBy(
            const RaylibRenderer &candidateRenderer) const noexcept;

        [[nodiscard]] const ::Mesh &getRawHandle() const noexcept;

        [[nodiscard]] ::Mesh &writableHandle() noexcept;

        [[nodiscard]] bool isLoaded() const noexcept;

        void untrackRenderer() noexcept;

    private:
        RaylibRenderer *owner;
        ::Mesh mesh;
        std::size_t vertices;
        std::size_t triangles;
        bool loaded = true;
    };

}
