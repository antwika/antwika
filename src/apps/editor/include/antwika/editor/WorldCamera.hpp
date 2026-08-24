#pragma once

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>

#include "antwika/editor/editor/CameraRig.hpp"
#include "antwika/editor/editor/PlaySession.hpp"

namespace antwika::editor
{

    [[nodiscard]] gfx::Mat4 getWorldRotation(const PlaySession &play);

    [[nodiscard]] gfx::Camera3D getWorldCamera(
        const PlaySession &play, const CameraRig &cameraRig);

}
