#pragma once

#include <cstdint>

#include <antwika/camera/FlyCamera.hpp>

namespace antwika::map
{

    struct CameraView final
    {
        camera::CameraTransform transform{};

        std::int32_t zoom = camera::kDefaultZoom;

        [[nodiscard]] bool operator==(const CameraView &other) const
            = default;
    };

}
