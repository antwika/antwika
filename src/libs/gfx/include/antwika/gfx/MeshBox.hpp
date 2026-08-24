#pragma once

#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"

namespace antwika::gfx
{

    struct MeshBox final
    {
        Vec3 lowPosition{};

        Vec3 highPosition{};

        [[nodiscard]] bool operator==(const MeshBox &other) const
            = default;
    };

    [[nodiscard]] MeshBox getMeshBox(const MeshData &mesh);

    [[nodiscard]] float getSpanFromBox(MeshBox box, Vec3 fromPosition);

    [[nodiscard]] bool isBoxBeyond(MeshBox box, Vec3 fromPosition, float reach);

    [[nodiscard]] bool isBoxOutside(MeshBox box, const Mat4 &clipMatrix);

}
