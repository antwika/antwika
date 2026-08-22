#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/ui/WidgetId.hpp>
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

    [[nodiscard]] std::string_view tabName(map::View view);

    [[nodiscard]] ui::WidgetId tabWidget(map::View view);

    [[nodiscard]] map::View viewAfterKey(
        map::View view, input::Key key, bool back);

    [[nodiscard]] std::uint32_t railWidth(
        gfx::Size windowSize, gfx::Size canvasSize);

    [[nodiscard]] std::uint32_t inspectColumnWidth(
        gfx::Size windowSize, gfx::Size canvasSize);

    [[nodiscard]] gfx::RectF inspectColumnBounds(gfx::Size canvasSize);

    inline constexpr float kGridZoomStep = 1.1F;

    inline constexpr float kMinGridZoom = 0.4F;

    inline constexpr float kMaxGridZoom = 8.0F;

    [[nodiscard]] gfx::RectF panZoomed(
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

    [[nodiscard]] gfx::RectF inspectedTileRect(gfx::Size canvasSize,
                                           tilemap::Tile tile);

    [[nodiscard]] gfx::RectF inspectedTileRect(gfx::RectF roomRect,
                                           tilemap::Tile tile);

    [[nodiscard]] gfx::RectF markerPlace(gfx::Size canvasSize,
                                         tilemap::TileEdge edge);

    [[nodiscard]] gfx::RectF markerPlace(gfx::RectF roomRect,
                                         tilemap::TileEdge edge);

    [[nodiscard]] gfx::RectF bothMarkerPlace(
        gfx::Size canvasSize, voxel::Side side);

    [[nodiscard]] gfx::RectF bothMarkerPlace(
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

    [[nodiscard]] constexpr EdgeToggle lastEnumerator(EdgeToggle) noexcept
    {
        return EdgeToggle::Forbidden;
    }

    inline constexpr std::array<EdgeToggle, enums::kCount<EdgeToggle>>
        kEveryEdgeToggle = enums::kAll<EdgeToggle>;

    inline constexpr float kEdgeToggleSide = 13.0F;

    [[nodiscard]] ui::WidgetId edgeToggleWidget(EdgeToggle whichToggle);

    inline constexpr ui::WidgetId kDeriveRulesWidget{178};

    [[nodiscard]] std::string_view edgeToggleName(EdgeToggle whichToggle);

    [[nodiscard]] gfx::RectF edgeTogglePlace(
        gfx::Size canvasSize, EdgeToggle whichToggle);

    [[nodiscard]] gfx::RectF edgeTogglePlace(
        gfx::RectF roomRect, EdgeToggle whichToggle);

    [[nodiscard]] std::optional<EdgeToggle> edgeToggleAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<EdgeToggle> edgeToggleAt(
        gfx::RectF roomRect, gfx::PointF point);

    [[nodiscard]] std::optional<tilemap::TileEdge> markerAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<tilemap::TileEdge> markerAt(
        gfx::RectF roomRect, gfx::PointF point);

    [[nodiscard]] gfx::RectF cornerPlace(
        gfx::Size canvasSize, voxel::Corner corner);

    [[nodiscard]] gfx::RectF cornerPlace(
        gfx::RectF roomRect, voxel::Corner corner);

    [[nodiscard]] std::optional<voxel::Corner> cornerAt(
        gfx::Size canvasSize, gfx::PointF point);

    [[nodiscard]] std::optional<voxel::Corner> cornerAt(
        gfx::RectF roomRect, gfx::PointF point);

    inline constexpr float kBorderThick = 1.5F;

    inline constexpr std::size_t kBorderSides = 4;

    [[nodiscard]] std::array<gfx::RectF, kBorderSides> outlineRects(
        gfx::RectF whereRect, float thickness);

    [[nodiscard]] std::optional<gfx::PointF> tileCenter(
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
