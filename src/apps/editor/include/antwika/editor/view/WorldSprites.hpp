#pragma once

#include <cstdint>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Math3D.hpp>

#include "antwika/editor/view/WorldRender.hpp"

namespace antwika::editor
{

    void drawSprite(
        const WorldRender &render,
        std::uint32_t tick,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::ITexture *sheetTexture,
        component::Position stoodPosition,
        component::AnimationState posedState);

}
