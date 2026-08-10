#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game::preview
{

    /**
     * @brief Draws a frame of the game through a renderer.
     *
     * @param renderer The renderer the frame is drawn through.
     * @param snapshot The scene to draw, as the game would hand it to
     *                 the renderer.
     * @param canvas The frame size in pixels.
     * @throws antwika::gfx::GfxError If a shipped atlas is missing or
     *         is not the size its sidecar records.
     */
    void drawScene(
        antwika::gfx::IRenderer &renderer,
        const SceneSnapshot &snapshot,
        antwika::gfx::Size canvas);

}
