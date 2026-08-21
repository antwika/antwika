#pragma once

#include <memory>

#include <antwika/character/Character.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::render
{

    class Sprites final
    {
    public:
        void open(gfx::ViewportRenderer &viewportRenderer);

        void drawCharacter(
            gfx::ViewportRenderer &viewportRenderer,
            gfx::IShader &shader,
            const gfx::Camera3D &camera,
            const gfx::Mat4 &modelMatrix,
            gfx::ITexture *sheetTexture,
            component::Position position,
            component::AnimationState posedState,
            time::Tick tick,
            const gfx::ITexture *lampShadowTexture) const;

        void drawShadow(
            gfx::ViewportRenderer &viewportRenderer,
            const gfx::Camera3D &camera,
            const gfx::Mat4 &modelMatrix,
            gfx::Vec3 position) const;

    private:
        std::unique_ptr<gfx::IMesh> figureMesh;
        std::unique_ptr<gfx::IMesh> shadowBlobMesh;
    };

}
