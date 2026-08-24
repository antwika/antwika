#include "antwika/editor/editor/CameraRig.hpp"

#include <cmath>
#include <cstdlib>

#include <antwika/camera/FlyCamera.hpp>

namespace antwika::editor
{

    void CameraRig::orbit(const float byYaw, const float byPitch)
    {
        const auto backDistance =
            viewHeight / std::tan(camera::kEditorFov / 2.0F);
        const auto eye = view.transform.position
                         - (camera::getForward(view.transform) * backDistance);

        view.transform =
            camera::getRotatedTransform(view.transform, byYaw, byPitch);
        view.transform.position =
            eye + (camera::getForward(view.transform) * backDistance);
    }


    void CameraRig::dragOrbit(
        const input::Position nowPosition,
        const input::Position lastPosition)
    {
        if (!orbitFromPosition.has_value())
        {
            return;
        }

        if (!orbiting
            && std::abs(nowPosition.x - orbitFromPosition->x)
                       + std::abs(nowPosition.y - orbitFromPosition->y)
                   > kOrbitSlack)
        {
            orbiting = true;
        }

        if (!orbiting)
        {
            return;
        }

        orbit(
            static_cast<float>(nowPosition.x - lastPosition.x)
                * camera::kMouseTurn,
            static_cast<float>(lastPosition.y - nowPosition.y)
                * camera::kMouseTurn);
    }

}
