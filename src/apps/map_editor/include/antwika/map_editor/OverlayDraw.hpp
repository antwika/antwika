#pragma once

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
        const tilemap::TileMap &map,
        geometry::GridCell cell,
        MarkerKind kind,
        const MapCamera &camera);

    void drawSelection(
        gfx::ViewportRenderer &view,
        const tilemap::TileMap &map,
        geometry::GridCell cell,
        const MapCamera &camera);

    void drawFreeMark(
        gfx::ViewportRenderer &view,
        const tilemap::TileMap &map,
        geometry::GridCell cell,
        const MapCamera &camera);

    void drawValidatorOverlay(
        gfx::ViewportRenderer &view,
        const EditorState &state,
        const MapCamera &camera);

}
