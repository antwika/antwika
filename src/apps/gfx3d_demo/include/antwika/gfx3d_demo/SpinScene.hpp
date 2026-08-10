#pragma once

#include <cstdint>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx3d_demo
{

    using antwika::gfx::Camera3D;
    using antwika::gfx::IMesh;
    using antwika::gfx::IRenderer;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Mat4;
    using antwika::gfx::Size;

    class SpinScene final
    {
    public:
        [[nodiscard]] Mat4 modelAt(std::uint64_t tick) const;

        [[nodiscard]] Camera3D cameraFor(Size canvas) const;

        void draw(
            IRenderer &renderer,
            const IMesh &cube,
            Size canvas,
            std::uint64_t tick) const;
    };

}
