#pragma once

#include <optional>

#include <antwika/gfx/Math3D.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/map/mapfile/CameraView.hpp>

namespace antwika::editor
{

    struct CameraRig final
    {
        map::CameraView view;

        float viewHeight = 0.0F;

        bool freeLook = false;

        bool panning = false;

        std::optional<gfx::Vec3> panGripPosition;

        bool orbiting = false;

        std::optional<input::Position> orbitFromPosition;
    };

}
