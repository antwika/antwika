#pragma once

#include <memory>

#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/MeshBox.hpp>

namespace antwika::render
{

    struct MeshPiece final
    {
        std::unique_ptr<gfx::IMesh> mesh;

        gfx::MeshBox box;
    };

}
