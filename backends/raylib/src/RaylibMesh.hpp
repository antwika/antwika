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
        RaylibMesh(RaylibRenderer &owner, ::Mesh mesh);

        RaylibMesh(const RaylibMesh &) = delete;
        RaylibMesh(RaylibMesh &&) = delete;

        RaylibMesh &operator=(const RaylibMesh &) = delete;
        RaylibMesh &operator=(RaylibMesh &&) = delete;

        ~RaylibMesh() override;

        [[nodiscard]] std::size_t vertexCount() const override;

        [[nodiscard]] std::size_t triangleCount() const override;

        [[nodiscard]] bool belongsTo(
            const RaylibRenderer &candidate) const noexcept;

        [[nodiscard]] const ::Mesh &raw() const noexcept;

        [[nodiscard]] bool isLoaded() const noexcept;

        void forgetRenderer() noexcept;

    private:
        RaylibRenderer *owner;
        ::Mesh mesh;
        std::size_t vertices;
        std::size_t triangles;
        bool loaded = true;
    };

}
