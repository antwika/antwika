#include "antwika/editor/ui/AtlasView.hpp"

#include <antwika/input/Key.hpp>
#include <antwika/gfx/SizeF.hpp>
#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/TilemapView.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::uint64_t kBoundaryToggleWidget = 176;

        constexpr std::uint64_t kForbiddenToggleWidget = 177;

        constexpr std::uint64_t kFirstTabWidget = 405;
    }

    map::View getViewAfterKey(
        const map::View view, const input::Key key, const bool back)
    {
        if (key == input::Key::Digit1)
        {
            return map::View::World;
        }

        if (key == input::Key::Digit2)
        {
            return map::View::Atlases;
        }

        if (key == input::Key::Digit3)
        {
            return map::View::Character;
        }

        if (key == input::Key::Digit4)
        {
            return map::View::Icons;
        }

        if (key == input::Key::Digit5)
        {
            return map::View::Plan;
        }

        if (key == input::Key::Tab)
        {
            const auto ways = enums::kCount<map::View>;

            return enums::wrapToEnum<map::View>(
                enums::index(view) + (back ? ways - 1 : 1));
        }

        return view;
    }

    std::string_view getTabName(const map::View view)
    {
        switch (view)
        {
        case map::View::World:
            return "World";
        case map::View::Atlases:
            return "Tiles";
        case map::View::Character:
            return "Characters";
        case map::View::Icons:
            return "Icons";
        case map::View::Plan:
            return "Plan";
        }

        return "";
    }

    widget::WidgetId getTabWidget(const map::View view)
    {
        return widget::WidgetId{
            kFirstTabWidget
            + static_cast<std::uint64_t>(view)};
    }

    std::uint32_t getRailWidth(
        const gfx::Size windowSize, const gfx::Size canvasSize)
    {
        if (canvasSize.width == 0)
        {
            return 0;
        }

        return static_cast<std::uint32_t>(
            kRightPanelWidth * static_cast<float>(windowSize.width)
            / static_cast<float>(canvasSize.width));
    }

    std::uint32_t getInspectColumnWidth(
        const gfx::Size windowSize, const gfx::Size canvasSize)
    {
        if (canvasSize.width == 0)
        {
            return 0;
        }

        return static_cast<std::uint32_t>(
            kInspectColumnWidth * static_cast<float>(windowSize.width)
            / static_cast<float>(canvasSize.width));
    }

    gfx::RectF getInspectColumnBounds(const gfx::Size canvasSize)
    {
        return gfx::RectF(
            gfx::PointF{
                static_cast<float>(canvasSize.width) - kRightPanelWidth
                    - kInspectColumnWidth,
                0.0F},
            gfx::SizeF{
                kInspectColumnWidth,
                static_cast<float>(canvasSize.height)});
    }

    gfx::RectF getPanZoomed(
        const gfx::RectF whereRect,
        const gfx::PointF panPoint,
        const float zoom)
    {
        const auto width = whereRect.size.width * zoom;
        const auto height = whereRect.size.height * zoom;

        return gfx::RectF(
            gfx::PointF{
                whereRect.originPoint.x + (
                (whereRect.size.width - width) / 2.0F)
                    + panPoint.x,
                whereRect.originPoint.y + (
                    (whereRect.size.height - height) / 2.0F)
                    + panPoint.y},
            gfx::SizeF{width, height});
    }

    namespace
    {
        constexpr float kHeadingText = 18.0F;

        constexpr float kMarkerGap = 3.0F;

        [[nodiscard]] bool liesAcross(const voxel::Side side)
        {
            return side == voxel::Side::Top || side == voxel::Side::Bottom;
        }

        [[nodiscard]] bool liesBefore(const voxel::Side side)
        {
            return side == voxel::Side::Top || side == voxel::Side::Left;
        }

        [[nodiscard]] float getBothMarkerReach()
        {
            return kMarkerGap + kMarkerThick + kMarkerGap;
        }

        [[nodiscard]] float reachOf(const voxel::EdgeKind edge)
        {
            return edge == voxel::EdgeKind::Interior
                         ? kMarkerGap
                         : getBothMarkerReach() + kBothMarkerSide
                             + kMarkerGap;
        }

        [[nodiscard]] float getMarkerReach()
        {
            return reachOf(voxel::EdgeKind::Boundary) + kMarkerThick;
        }

        [[nodiscard]] gfx::RectF frameOf(const gfx::RectF roomRect)
        {
            const auto cell = tilemap::getGridCellSize();
            const auto width =
                static_cast<float>(cell.width) * kInspectedScale;
            const auto scaledHeight =
                static_cast<float>(cell.height) * kInspectedScale;

            return gfx::RectF(
                gfx::PointF{
                    roomRect.originPoint.x
                        + ((roomRect.size.width - width) / 2.0F),
                    roomRect.originPoint.y + kPaneMargin + kHeadingText
                        + getMarkerReach()},
                gfx::SizeF{width, scaledHeight});
        }
    }

    gfx::RectF getInspectedTileRect(
        const gfx::Size canvasSize, const tilemap::Tile tile)
    {
        return getInspectedTileRect(getInspectColumnBounds(canvasSize), tile);
    }

    gfx::RectF getInspectedTileRect(
        const gfx::RectF roomRect, const tilemap::Tile tile)
    {
        const auto frame = frameOf(roomRect);
        const auto tileSize = tilemap::tileSizeOf(tile.atlas);
        const auto width =
            static_cast<float>(tileSize.width) * kInspectedScale;
        const auto scaledHeight =
            static_cast<float>(tileSize.height) * kInspectedScale;

        return gfx::RectF(
            gfx::PointF{
                frame.originPoint.x + ((frame.size.width - width) / 2.0F),
                frame.originPoint.y + (
                (frame.size.height - scaledHeight) / 2.0F)},
            gfx::SizeF{width, scaledHeight});
    }

    gfx::RectF getMarkerPlace(
        const gfx::Size canvasSize, const tilemap::TileEdge edge)
    {
        return getMarkerPlace(getInspectColumnBounds(canvasSize), edge);
    }

    gfx::RectF getMarkerPlace(
        const gfx::RectF roomRect, const tilemap::TileEdge edge)
    {
        const auto frame = frameOf(roomRect);
        const auto alongLength =
            liesAcross(edge.side) ? frame.size.width
                                  : frame.size.height;
        const auto length =
            edge.edge == voxel::EdgeKind::Interior
                       ? alongLength * kInwardMarkerLengthFraction
                       : alongLength;
        const auto reach = reachOf(edge.edge);
        const auto edgeOffset =
            liesBefore(edge.side) ? -reach - kMarkerThick : reach;

        if (liesAcross(edge.side))
        {
            const auto side = liesBefore(edge.side)
                            ? frame.originPoint.y
                            : frame.originPoint.y + frame.size.height;

            return gfx::RectF(
                gfx::PointF{
                    frame.originPoint.x
                        + ((frame.size.width - length) / 2.0F),
                    side + edgeOffset},
                gfx::SizeF{length, kMarkerThick});
        }

        const auto side = liesBefore(edge.side)
                        ? frame.originPoint.x
                        : frame.originPoint.x + frame.size.width;

        return gfx::RectF(
            gfx::PointF{
                side + edgeOffset,
                frame.originPoint.y
                    + ((frame.size.height - length) / 2.0F)},
            gfx::SizeF{kMarkerThick, length});
    }

    EdgeSelection edgeSelectionOf(const tilemap::TileEdge edge)
    {
        return EdgeSelection{.side = edge.side, .edge = edge.edge};
    }

    EdgeSelection bothEdgesOf(const voxel::Side side)
    {
        return EdgeSelection{.side = side, .edge = std::nullopt};
    }

    bool covers(const EdgeSelection selection, const tilemap::TileEdge edge)
    {
        return selection.side == edge.side
               && (!selection.edge.has_value()
                   || *selection.edge == edge.edge);
    }

    std::vector<tilemap::TileEdge> edgesIn(const EdgeSelection selection)
    {
        std::vector<tilemap::TileEdge> edges;

        for (const auto edge : tilemap::kEveryTileEdge)
        {
            if (covers(selection, edge))
            {
                edges.push_back(edge);
            }
        }

        return edges;
    } // GCOVR_EXCL_LINE

    gfx::RectF getBothMarkerPlace(
        const gfx::Size canvasSize, const voxel::Side side)
    {
        return getBothMarkerPlace(getInspectColumnBounds(canvasSize), side);
    }

    gfx::RectF getBothMarkerPlace(
        const gfx::RectF roomRect,
        const voxel::Side side)
    {
        const auto frame = frameOf(roomRect);
        const auto reach = getBothMarkerReach();
        const auto edgeOffset =
            liesBefore(side) ? -reach - kBothMarkerSide : reach;

        if (liesAcross(side))
        {
            const auto alongOrigin = liesBefore(side)
                                   ? frame.originPoint.y
                                   : frame.originPoint.y
                                         + frame.size.height;

            return gfx::RectF(
                gfx::PointF{
                    frame.originPoint.x
                        + ((frame.size.width - kBothMarkerSide)
                           / 2.0F),
                    alongOrigin + edgeOffset},
                gfx::SizeF{kBothMarkerSide, kBothMarkerSide});
        }

        const auto alongOrigin = liesBefore(side)
                               ? frame.originPoint.x
                               : frame.originPoint.x + frame.size.width;

        return gfx::RectF(
            gfx::PointF{
                alongOrigin + edgeOffset,
                frame.originPoint.y
                    + ((frame.size.height - kBothMarkerSide)
                       / 2.0F)},
            gfx::SizeF{kBothMarkerSide, kBothMarkerSide});
    }

    std::optional<voxel::Side> bothMarkerAt(
        const gfx::Size canvasSize, const gfx::PointF point)
    {
        return bothMarkerAt(getInspectColumnBounds(canvasSize), point);
    }

    std::optional<voxel::Side> bothMarkerAt(
        const gfx::RectF roomRect, const gfx::PointF point)
    {
        for (const auto side : voxel::kEverySide)
        {
            if (holds(getBothMarkerPlace(roomRect, side), point))
            {
                return side;
            }
        }

        return std::nullopt;
    }

    gfx::RectF getCornerPlace(
        const gfx::Size canvasSize, const voxel::Corner corner)
    {
        return getCornerPlace(getInspectColumnBounds(canvasSize), corner);
    }

    gfx::RectF getCornerPlace(
        const gfx::RectF roomRect, const voxel::Corner corner)
    {
        const auto frame = frameOf(roomRect);
        const auto reach = kMarkerGap + kMarkerThick;
        const auto left = corner == voxel::Corner::TopLeft
                          || corner == voxel::Corner::BottomLeft;
        const auto topCorner = corner == voxel::Corner::TopLeft
                          || corner == voxel::Corner::TopRight;

        return gfx::RectF(
            gfx::PointF{
                left ? frame.originPoint.x - reach
                     : frame.originPoint.x + frame.size.width
                           + kMarkerGap,
                topCorner ? frame.originPoint.y - reach
                     : frame.originPoint.y + frame.size.height
                           + kMarkerGap},
            gfx::SizeF{kMarkerThick, kMarkerThick});
    }

    std::optional<voxel::Corner> cornerAt(
        const gfx::Size canvasSize, const gfx::PointF point)
    {
        return cornerAt(getInspectColumnBounds(canvasSize), point);
    }

    std::optional<voxel::Corner> cornerAt(
        const gfx::RectF roomRect, const gfx::PointF point)
    {
        for (const auto corner : voxel::kEveryCorner)
        {
            if (holds(getCornerPlace(roomRect, corner), point))
            {
                return corner;
            }
        }

        return std::nullopt;
    }

    widget::WidgetId getEdgeToggleWidget(const EdgeToggle whichToggle)
    {
        return widget::WidgetId{
            whichToggle == EdgeToggle::Boundary ? kBoundaryToggleWidget
                         : kForbiddenToggleWidget};
    }

    std::string_view getEdgeToggleName(const EdgeToggle whichToggle)
    {
        return whichToggle == EdgeToggle::Boundary ? "rim" : "shut";
    }

    gfx::RectF getEdgeTogglePlace(
        const gfx::Size canvasSize, const EdgeToggle whichToggle)
    {
        return getEdgeTogglePlace(getInspectColumnBounds(canvasSize), whichToggle);
    }

    gfx::RectF getEdgeTogglePlace(
        const gfx::RectF roomRect, const EdgeToggle whichToggle)
    {
        const auto frame = frameOf(roomRect);
        const auto rank = whichToggle == EdgeToggle::Boundary ? 0.0F : 1.0F;
        const auto span = kEdgeToggleSide + kMarkerGap;

        return gfx::RectF(
            gfx::PointF{
                frame.originPoint.x + (frame.size.width / 2.0F)
                    - span + (rank * span),
                frame.originPoint.y + frame.size.height + getMarkerReach()
                    + kMarkerGap},
            gfx::SizeF{kEdgeToggleSide, kEdgeToggleSide});
    }

    std::optional<EdgeToggle> edgeToggleAt(
        const gfx::Size canvasSize, const gfx::PointF point)
    {
        return edgeToggleAt(getInspectColumnBounds(canvasSize), point);
    }

    std::optional<EdgeToggle> edgeToggleAt(
        const gfx::RectF roomRect, const gfx::PointF point)
    {
        for (const auto which : kEveryEdgeToggle)
        {
            if (holds(getEdgeTogglePlace(roomRect, which), point))
            {
                return which;
            }
        }

        return std::nullopt;
    }

    std::optional<tilemap::TileEdge> markerAt(
        const gfx::Size canvasSize, const gfx::PointF point)
    {
        return markerAt(getInspectColumnBounds(canvasSize), point);
    }

    std::optional<tilemap::TileEdge> markerAt(
        const gfx::RectF roomRect, const gfx::PointF point)
    {
        for (const auto edge : tilemap::kEveryTileEdge)
        {
            if (holds(getMarkerPlace(roomRect, edge), point))
            {
                return edge;
            }
        }

        return std::nullopt;
    }

    std::optional<geometry::GridCell> cellShownAt(
        const tilemap::Tilemap &tilemap,
        const gfx::RectF whereRect,
        const gfx::RectF shownRect,
        const gfx::PointF point)
    {
        if (!holds(shownRect, point))
        {
            return std::nullopt;
        }

        return getCellAtPoint(tilemap, whereRect, point);
    }

    GestureResult gestureFrom(
        const tilemap::Tilemap &tilemap,
        const gfx::Size canvasSize,
        const gfx::RectF whereRect,
        const std::optional<gfx::PointF> pressedAtPoint,
        const gfx::PointF releasedAtPoint,
        const bool looking,
        const std::optional<EdgeSelection> settlingSelection)
    {
        return gestureFrom(
            tilemap,
            getInspectColumnBounds(canvasSize),
            whereRect,
            getTilemapBounds(canvasSize),
            pressedAtPoint,
            releasedAtPoint,
            looking,
            settlingSelection);
    }

    GestureResult gestureFrom(
        const tilemap::Tilemap &tilemap,
        const gfx::RectF roomRect,
        const gfx::RectF whereRect,
        const gfx::RectF shownRect,
        const std::optional<gfx::PointF> pressedAtPoint,
        const gfx::PointF releasedAtPoint,
        const bool looking,
        const std::optional<EdgeSelection> settlingSelection)
    {
        if (!pressedAtPoint.has_value())
        {
            return GestureResult{};
        }

        const auto fromCell = cellShownAt(
            tilemap,
            whereRect,
            shownRect,
            *pressedAtPoint);
        const auto ontoCell = cellShownAt(
            tilemap,
            whereRect,
            shownRect,
            releasedAtPoint);

        if (fromCell.has_value() && ontoCell.has_value())
        {
            if (*fromCell != *ontoCell)
            {
                return GestureResult{
                    .action = PointerAction::Swap,
                    .fromCell = *fromCell,
                    .toCell = *ontoCell};
            }

            return GestureResult{
                .action = looking && settlingSelection.has_value()
                        ? PointerAction::Rule
                        : PointerAction::Look,
                .toCell = *ontoCell};
        }

        if (fromCell.has_value() || ontoCell.has_value() || !looking)
        {
            return GestureResult{};
        }

        const auto pressedCorner = cornerAt(roomRect, *pressedAtPoint);

        if (pressedCorner.has_value()
            && pressedCorner == cornerAt(roomRect, releasedAtPoint))
        {
            return GestureResult{
                .action = PointerAction::Turn, .corner = *pressedCorner};
        }

        const auto pressedPair = bothMarkerAt(roomRect, *pressedAtPoint);

        if (pressedPair.has_value()
            && pressedPair == bothMarkerAt(roomRect, releasedAtPoint))
        {
            return GestureResult{
                .action = PointerAction::PixelSelection,
                .selection = bothEdgesOf(*pressedPair)};
        }

        const auto pressedMarker = markerAt(roomRect, *pressedAtPoint);

        if (!pressedMarker.has_value()
            || pressedMarker != markerAt(roomRect, releasedAtPoint))
        {
            return GestureResult{};
        }

        return GestureResult{
            .action = PointerAction::PixelSelection,
            .selection = edgeSelectionOf(*pressedMarker)};
    }

    std::array<gfx::RectF, kBorderSides> getOutlineRects(
        const gfx::RectF whereRect, const float thickness)
    {
        const auto width = whereRect.size.width + (2.0F * thickness);

        return {
            gfx::RectF(
                gfx::PointF{
                    whereRect.originPoint.x - thickness,
                    whereRect.originPoint.y - thickness},
                gfx::SizeF{width, thickness}),
            gfx::RectF(
                gfx::PointF{
                    whereRect.originPoint.x - thickness,
                    whereRect.originPoint.y + whereRect.size.height},
                gfx::SizeF{width, thickness}),
            gfx::RectF(
                gfx::PointF{
                    whereRect.originPoint.x - thickness,
                    whereRect.originPoint.y},
                gfx::SizeF{thickness, whereRect.size.height}),
            gfx::RectF(
                gfx::PointF{
                    whereRect.originPoint.x + whereRect.size.width,
                    whereRect.originPoint.y},
                gfx::SizeF{thickness, whereRect.size.height})};
    } // GCOVR_EXCL_LINE

    std::optional<gfx::PointF> getTileCenter(
        const tilemap::Tilemap &tilemap,
        const gfx::RectF whereRect,
        const tilemap::Tile tile)
    {
        const auto sits = tilemap::getCellHoldingTile(tilemap, tile);

        if (!sits.has_value())
        {
            return std::nullopt;
        }

        const auto place =
            getTilePlace(tilemap, sits->column, sits->row, whereRect);

        return gfx::PointF{
            place.originPoint.x + (place.size.width / 2.0F),
            place.originPoint.y + (place.size.height / 2.0F)};
    }

}
