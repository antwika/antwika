#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/widget/WidgetId.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include <antwika/map/Settings.hpp>

#include "antwika/editor/ui/EdgeSelection.hpp"
#include "antwika/editor/ui/GestureResult.hpp"
#include "antwika/editor/ui/PointerAction.hpp"

namespace antwika::editor
{

    [[nodiscard]] std::string_view getTabName(map::View view);

    [[nodiscard]] widget::WidgetId getTabWidget(map::View view);

    [[nodiscard]] map::View getViewAfterKey(
        map::View view, input::Key key, bool back);

    [[nodiscard]] std::uint32_t getRailWidth(
        gfx::Size windowSize, gfx::Size canvasSize);

    [[nodiscard]] std::uint32_t getInspectColumnWidth(
        gfx::Size windowSize, gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF getInspectColumnBounds(gfx::Size canvasSize);

    inline constexpr float kGridZoomStep = 1.1F;

    inline constexpr float kMinGridZoom = 0.4F;

    inline constexpr float kMaxGridZoom = 8.0F;

    [[nodiscard]] gfx::RectF getPanZoomed(
        gfx::RectF whereRect, gfx::PointF panPoint, float zoom);

    inline constexpr float kInspectedScale = 3.0F;

    inline constexpr float kMarkerThick = 6.0F;

    inline constexpr float kInwardMarkerLengthFraction = 0.5F;

    inline constexpr float kBothMarkerSide = 6.0F;

    [[nodiscard]] EdgeSelection edgeSelectionOf(tilemap::TileEdge edge);

    [[nodiscard]] EdgeSelection bothEdgesOf(voxel::Side side);

    [[nodiscard]] bool covers(EdgeSelection selection, tilemap::TileEdge edge);

    [[nodiscard]] std::vector<tilemap::TileEdge> edgesIn(
        EdgeSelection selection);

    [[nodiscard]] gfx::RectF getInspectedTileRect(gfx::Size canvasSize,
                                           tilemap::Tile tile);

    [[nodiscard]] gfx::RectF getInspectedTileRect(gfx::RectF roomRect,
                                           tilemap::Tile tile);

    [[nodiscard]] gfx::RectF getMarkerPlace(gfx::Size canvasSize,
                                         tilemap::TileEdge edge);

    [[nodiscard]] gfx::RectF getMarkerPlace(gfx::RectF roomRect,
                                         tilemap::TileEdge edge);

    [[nodiscard]] gfx::RectF getBothMarkerPlace(
        gfx::Size canvasSize, voxel::Side side);

    [[nodiscard]] gfx::RectF getBothMarkerPlace(
        gfx::RectF roomRect, voxel::Side side);

    [[nodiscard]] std::optional<voxel::Side> bothMarkerAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<voxel::Side> bothMarkerAt(
        gfx::RectF roomRect, gfx::PointF point);

    enum class EdgeToggle : std::uint8_t
    {
        Boundary,
        Forbidden,
    };

    [[nodiscard]] constexpr EdgeToggle getLastEnumerator(EdgeToggle) noexcept
    {
        return EdgeToggle::Forbidden;
    }

    inline constexpr std::array<EdgeToggle, enums::kCount<EdgeToggle>>
        kEveryEdgeToggle = enums::kAll<EdgeToggle>;

    inline constexpr float kEdgeToggleSide = 13.0F;

    [[nodiscard]] widget::WidgetId getEdgeToggleWidget(EdgeToggle whichToggle);

    inline constexpr widget::WidgetId kDeriveRulesWidget{178};

    [[nodiscard]] std::string_view getEdgeToggleName(EdgeToggle whichToggle);

    [[nodiscard]] gfx::RectF getEdgeTogglePlace(
        gfx::Size canvasSize, EdgeToggle whichToggle);

    [[nodiscard]] gfx::RectF getEdgeTogglePlace(
        gfx::RectF roomRect, EdgeToggle whichToggle);

    [[nodiscard]] std::optional<EdgeToggle> edgeToggleAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<EdgeToggle> edgeToggleAt(
        gfx::RectF roomRect, gfx::PointF point);

    [[nodiscard]] std::optional<tilemap::TileEdge> markerAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<tilemap::TileEdge> markerAt(
        gfx::RectF roomRect, gfx::PointF point);

    [[nodiscard]] gfx::RectF getCornerPlace(
        gfx::Size canvasSize, voxel::Corner corner);

    [[nodiscard]] gfx::RectF getCornerPlace(
        gfx::RectF roomRect, voxel::Corner corner);

    [[nodiscard]] std::optional<voxel::Corner> cornerAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<voxel::Corner> cornerAt(
        gfx::RectF roomRect, gfx::PointF point);

    inline constexpr float kBorderThick = 1.5F;

    inline constexpr std::size_t kBorderSides = 4;

    [[nodiscard]] std::array<gfx::RectF, kBorderSides> getOutlineRects(
        gfx::RectF whereRect, float thickness);

    [[nodiscard]] std::optional<gfx::PointF> getTileCenter(
        const tilemap::Tilemap &tilemap,
        gfx::RectF whereRect,
        tilemap::Tile tile);

    [[nodiscard]] std::optional<geometry::GridCell> cellShownAt(
        const tilemap::Tilemap &tilemap,
        gfx::RectF whereRect,
        gfx::RectF shownRect,
        gfx::PointF point);

    [[nodiscard]] GestureResult gestureFrom(
        const tilemap::Tilemap &tilemap,
        gfx::Size canvasSize,
        gfx::RectF whereRect,
        std::optional<gfx::PointF> pressedAtPoint,
        gfx::PointF releasedAtPoint,
        bool looking,
        std::optional<EdgeSelection> settlingSelection);

    [[nodiscard]] GestureResult gestureFrom(
        const tilemap::Tilemap &tilemap,
        gfx::RectF roomRect,
        gfx::RectF whereRect,
        gfx::RectF shownRect,
        std::optional<gfx::PointF> pressedAtPoint,
        gfx::PointF releasedAtPoint,
        bool looking,
        std::optional<EdgeSelection> settlingSelection);

}
