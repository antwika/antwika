#include "antwika/editor/WorldCamera.hpp"

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::editor
{

    gfx::Mat4 getWorldRotation(const PlaySession &play)
    {
        const auto orientation =
            play.game->getWorld().get<component::Orientation>(
                play.game->getEye());

        return voxelmap::getModelRotation(orientation.yaw, orientation.pitch);
    }

    gfx::Camera3D getWorldCamera(
        const PlaySession &play, const CameraRig &cameraRig)
    {
        return play.playing
                   ? camera::cameraOf(
                         play.game->getCameraTransform(),
                         camera::kCanvasSize,
                         cameraRig.viewHeight)
                   : camera::perspectiveOf(
                       cameraRig.view.transform,
                       camera::kCanvasSize,
                       cameraRig.viewHeight);
    }

}
