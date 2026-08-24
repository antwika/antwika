#include "antwika/editor/view/WorldSprites.hpp"

namespace antwika::editor
{

    void drawSprite(
        const WorldRender &render,
        const std::uint32_t tick,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::ITexture *const sheetTexture,
        const component::Position stoodPosition,
        const component::AnimationState posedState)
    {
        render.sprites.drawCharacter(
            render.viewportRenderer,
            render.worldShader.getProgram(),
            camera,
            modelMatrix,
            sheetTexture,
            stoodPosition,
            posedState,
            tick,
            render.lightPasses.getLampShadows());
    }

}
