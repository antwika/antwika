#include "antwika/map_editor/OverlayDraw.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/autotile/SheetLayout.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/SizeF.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/PaletteMath.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::autotile::kLevelRise;
        using antwika::autotile::kUnit;
        using antwika::geometry::GridCell;
        using antwika::gfx::Color;
        using antwika::gfx::PointF;
        using antwika::gfx::RectF;
        using antwika::gfx::SizeF;
        using antwika::gfx::ViewportRenderer;
        using antwika::tilemap::TileMap;

        constexpr std::array<Color, kMarkerKindCount> kMarkerInks{
            Color{.red = 255, .green = 128, .blue = 0},
            Color{.red = 64, .green = 64, .blue = 255},
            Color{.red = 255, .green = 0, .blue = 255},
            Color{.red = 255, .green = 255, .blue = 0},
            Color{.red = 0, .green = 255, .blue = 255},
            Color{.red = 0, .green = 255, .blue = 0}};

        constexpr Color kUnreachable{
            .red = 255, .green = 0, .blue = 0, .alpha = 96};
        constexpr Color kFinding{.red = 255, .green = 0, .blue = 0};
        constexpr Color kSelected{
            .red = 255, .green = 255, .blue = 255};

        constexpr float kUnitF = static_cast<float>(kUnit);

        [[nodiscard]] PointF cellOrigin(
            const TileMap &map,
            const GridCell cell,
            const MapCamera &camera)
        {
            const auto zoom = camera.zoom();
            const auto mapX =
                static_cast<float>(cell.column) * kUnitF;
            const auto mapY =
                static_cast<float>(cell.row) * kUnitF
                - static_cast<float>(map.at(cell).height)
                      * static_cast<float>(kLevelRise);

            return {
                mapX * zoom + camera.panX,
                mapY * zoom + camera.panY
                    + static_cast<float>(kMenuBarHeight)};
        }

        void outlineRect(
            ViewportRenderer &view,
            const RectF rect,
            const Color color)
        {
            const auto left = rect.origin.x;
            const auto top = rect.origin.y;
            const auto right = left + rect.size.width;
            const auto bottom = top + rect.size.height;

            view.drawLine({left, top}, {right, top}, color);
            view.drawLine({right, top}, {right, bottom}, color);
            view.drawLine({right, bottom}, {left, bottom}, color);
            view.drawLine({left, bottom}, {left, top}, color);
        }

        [[nodiscard]] bool walkable(
            const TileMap &map, const GridCell cell)
        {
            const auto &value = map.at(cell);

            if (value.terrain == tilemap::TerrainClass::Wall)
            {
                return false;
            }

            return value.terrain != tilemap::TerrainClass::Water
                   || value.overlay == tilemap::Overlay::Bridge;
        }

        [[nodiscard]] RectF cellRectF(
            const TileMap &map,
            const GridCell cell,
            const MapCamera &camera)
        {
            const auto zoom = camera.zoom();

            return {
                cellOrigin(map, cell, camera),
                SizeF{kUnitF * zoom, kUnitF * zoom}};
        }
    }

    void drawMarker(
        ViewportRenderer &view,
        const TileMap &map,
        const GridCell cell,
        const MarkerKind kind,
        const MapCamera &camera)
    {
        const auto zoom = camera.zoom();
        const auto origin = cellOrigin(map, cell, camera);
        const RectF marker(
            {origin.x + 6.0F * zoom, origin.y + 6.0F * zoom},
            {4.0F * zoom, 4.0F * zoom});
        const auto ink =
            kMarkerInks[static_cast<std::size_t>(kind)
                        % kMarkerKindCount];

        if (kind == MarkerKind::Trigger)
        {
            outlineRect(view, marker, ink);
            return;
        }

        view.drawRect(marker, ink);
    }

    void drawSelection(
        ViewportRenderer &view,
        const TileMap &map,
        const GridCell cell,
        const MapCamera &camera)
    {
        outlineRect(view, cellRectF(map, cell, camera), kSelected);
    }

    void drawFreeMark(
        ViewportRenderer &view,
        const TileMap &map,
        const GridCell cell,
        const MapCamera &camera)
    {
        const auto zoom = camera.zoom();
        const auto origin = cellOrigin(map, cell, camera);
        const auto tick = zoom >= 1.0F ? 2.0F * zoom : 1.0F;

        view.drawRect(
            RectF(
                {origin.x + 1.0F * zoom, origin.y + 1.0F * zoom},
                {tick, tick}),
            chromeFor(map.header().paper).freeMark);
    }

    void drawValidatorOverlay(
        ViewportRenderer &view,
        const EditorState &state,
        const MapCamera &camera)
    {
        if (!state.overlayOn || !state.report.has_value())
        {
            return;
        }

        const auto &map = state.map;
        const auto &reachable = state.report->reachable;

        for (std::uint32_t row = 0; row < map.rows(); ++row)
        {
            for (std::uint32_t column = 0; column < map.columns();
                 ++column)
            {
                const auto cell =
                    GridCell{.column = column, .row = row};
                const auto index =
                    static_cast<std::size_t>(row) * map.columns()
                    + column;

                if (index >= reachable.size() || reachable[index])
                {
                    continue;
                }

                if (!walkable(map, cell))
                {
                    continue;
                }

                view.drawRect(
                    cellRectF(map, cell, camera), kUnreachable);
            }
        }

        for (const auto &finding : state.report->findings)
        {
            if (!finding.at.has_value())
            {
                continue;
            }

            outlineRect(
                view,
                cellRectF(map, *finding.at, camera),
                kFinding);
        }
    }

}
