#pragma once

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game::preview
{

    /**
     * @brief Draws a frame of the game into a bitmap.
     *
     * @param snapshot The scene to draw, as the game would hand it to
     *                 the renderer.
     * @param canvas The page size in pixels.
     * @return The page, with the scene drawn on it.
     * @throws antwika::gfx::GfxError If a shipped atlas is missing or
     *         is not the size its sidecar records.
     */
    [[nodiscard]] antwika::gfx::Bitmap paintedScene(
        const SceneSnapshot &snapshot, antwika::gfx::Size canvas);

}
