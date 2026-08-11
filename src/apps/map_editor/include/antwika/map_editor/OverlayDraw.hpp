#pragma once

#include <cstdint>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    void drawMarker(
        gfx::ViewportRenderer &view,
        geometry::GridCell cell,
        std::int32_t lift,
        MarkerKind kind,
        const MapCamera &camera);

    void drawSelection(
        gfx::ViewportRenderer &view,
        geometry::GridCell cell,
        std::int32_t lift,
        const MapCamera &camera);

    /**
     * @brief Outlines the hovered cell in the focus-ring yellow.
     *
     * Ensures: the outline thickness is one canvas pixel at 1x
     *          zoom and scales with the camera, and the rectangle
     *          lifts by the given level like the selection
     *          outline.
     */
    void drawHover(
        gfx::ViewportRenderer &view,
        geometry::GridCell cell,
        std::int32_t lift,
        const MapCamera &camera);

    void drawFreeMark(
        gfx::ViewportRenderer &view,
        const tilemap::TileMap &map,
        geometry::GridCell cell,
        std::int32_t lift,
        const MapCamera &camera);

    void drawValidatorOverlay(
        gfx::ViewportRenderer &view,
        const EditorState &state,
        const MapCamera &camera);

    /**
     * @brief Draws the map marquee and selection outline.
     *
     * Ensures: a marquee in progress draws dashed, a placed
     *          selection draws solid, a move in progress draws the
     *          outline displaced by the drag delta, and every
     *          outline lifts by the active level.
     */
    void drawMapSelectionOverlay(
        gfx::ViewportRenderer &view, const EditorStore &store);

    /**
     * @brief Outlines every column holding the active level's slab.
     *
     * Ensures: nothing is drawn while the active level is zero, and
     *          each outline sits on the active level's lifted plane.
     */
    void drawActiveLevelMarks(
        gfx::ViewportRenderer &view,
        const EditorState &state,
        const MapCamera &camera);

}
