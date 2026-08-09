#pragma once

#include <memory>

#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"

namespace antwika::gfx
{

    class IRenderer3D
    {
    public:
        virtual ~IRenderer3D() = default;

        [[nodiscard]] virtual std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) = 0;

        virtual void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) = 0;
    };

}
