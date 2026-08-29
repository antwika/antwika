#pragma once

#include <cstdint>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/gfx/Math3D.hpp>

namespace antwika::gameplay
{

    class ICameraRig
    {
    public:
        ICameraRig() = default;

        virtual ~ICameraRig() = default;

        ICameraRig(const ICameraRig &) = delete;
        ICameraRig(ICameraRig &&) = delete;

        ICameraRig &operator=(const ICameraRig &) = delete;
        ICameraRig &operator=(ICameraRig &&) = delete;

        [[nodiscard]] virtual camera::CameraTransform &getCameraTransform()
            noexcept = 0;

        [[nodiscard]] virtual const camera::CameraTransform &getCameraTransform()
            const noexcept = 0;

        [[nodiscard]] virtual std::int32_t getZoom() const noexcept = 0;

        virtual void setZoom(std::int32_t zoom) noexcept = 0;

        [[nodiscard]] virtual gfx::Vec3 getCameraTarget() const noexcept = 0;

        virtual void setCameraTarget(gfx::Vec3 targetPosition) noexcept = 0;

        virtual void aimAt(
            const gfx::Mat4 &modelMatrix, gfx::Vec3 position) = 0;

        virtual void follow(
            const gfx::Mat4 &modelMatrix, gfx::Vec3 position) = 0;
    };

}
