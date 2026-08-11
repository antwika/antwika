#include "antwika/map_editor/OverlayDraw.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/autotile/Metrics.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/SizeF.hpp>
#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/PaletteMath.hpp"
#include "antwika/map_editor/Selection.hpp"

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
        using antwika::tilemap::Column;
        using antwika::tilemap::Slab;

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
            const GridCell cell,
            const std::int32_t lift,
            const MapCamera &camera)
        {
            const auto zoom = camera.zoom();
            const auto mapX =
                static_cast<float>(cell.column) * kUnitF;
            const auto mapY =
                static_cast<float>(cell.row) * kUnitF
                - static_cast<float>(lift)
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

        [[nodiscard]] bool walkableSlab(const Slab &slab)
        {
            if (slab.terrain == tilemap::TerrainClass::Wall)
            {
                return false;
            }

            return slab.terrain != tilemap::TerrainClass::Water
                   || slab.overlay == tilemap::Overlay::Bridge;
        }

        [[nodiscard]] std::int32_t surfaceLift(const Column &column)
        {
            const auto &slabs = column.slabs();

            for (auto slab = slabs.rbegin(); slab != slabs.rend();
                 ++slab)
            {
                if (column.standable(slab->level)
                    && walkableSlab(*slab))
                {
                    return slab->level;
                }
            }

            const auto *top = column.top();

            return top != nullptr ? top->level : 0;
        }

        [[nodiscard]] RectF cellRectF(
            const GridCell cell,
            const std::int32_t lift,
            const MapCamera &camera)
        {
            const auto zoom = camera.zoom();

            return {
                cellOrigin(cell, lift, camera),
                SizeF{kUnitF * zoom, kUnitF * zoom}};
        }

        [[nodiscard]] RectF spanRectF(
            const CellSpan &span,
            const std::int32_t lift,
            const MapCamera &camera)
        {
            const auto zoom = camera.zoom();

            return {
                cellOrigin(span.origin, lift, camera),
                SizeF{
                    static_cast<float>(span.columns) * kUnitF
                        * zoom,
                    static_cast<float>(span.rows) * kUnitF
                        * zoom}};
        }

        constexpr float kMarqueeDashOn = 3.0F;

        constexpr float kMarqueeDashPeriod = 6.0F;

        void dashedRect(
            ViewportRenderer &view,
            const RectF rect,
            const Color color)
        {
            const auto right = rect.origin.x + rect.size.width;
            const auto bottom = rect.origin.y + rect.size.height;

            for (auto x = rect.origin.x; x < right;
                 x += kMarqueeDashPeriod)
            {
                const auto width =
                    std::min(kMarqueeDashOn, right - x);

                view.drawRect(
                    RectF({x, rect.origin.y}, {width, 1.0F}),
                    color);
                view.drawRect(
                    RectF({x, bottom - 1.0F}, {width, 1.0F}),
                    color);
            }

            for (auto y = rect.origin.y; y < bottom;
                 y += kMarqueeDashPeriod)
            {
                const auto height =
                    std::min(kMarqueeDashOn, bottom - y);

                view.drawRect(
                    RectF({rect.origin.x, y}, {1.0F, height}),
                    color);
                view.drawRect(
                    RectF({right - 1.0F, y}, {1.0F, height}),
                    color);
            }
        }
    }

    void drawMarker(
        ViewportRenderer &view,
        const GridCell cell,
        const std::int32_t lift,
        const MarkerKind kind,
        const MapCamera &camera)
    {
        const auto zoom = camera.zoom();
        const auto origin = cellOrigin(cell, lift, camera);
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
        const GridCell cell,
        const std::int32_t lift,
        const MapCamera &camera)
    {
        outlineRect(view, cellRectF(cell, lift, camera), kSelected);
    }

    void drawHover(
        ViewportRenderer &view,
        const GridCell cell,
        const std::int32_t lift,
        const MapCamera &camera)
    {
        const auto rect = cellRectF(cell, lift, camera);
        const auto thickness =
            camera.zoom() >= 1.0F ? camera.zoom() : 1.0F;
        const auto color = antwika::ui::Theme{}.focusRing;
        const auto left = rect.origin.x;
        const auto top = rect.origin.y;
        const auto width = rect.size.width;
        const auto height = rect.size.height;

        view.drawRect(
            RectF({left, top}, {width, thickness}), color);
        view.drawRect(
            RectF(
                {left, top + height - thickness},
                {width, thickness}),
            color);
        view.drawRect(
            RectF(
                {left, top + thickness},
                {thickness, height - 2.0F * thickness}),
            color);
        view.drawRect(
            RectF(
                {left + width - thickness, top + thickness},
                {thickness, height - 2.0F * thickness}),
            color);
    }

    void drawFreeMark(
        ViewportRenderer &view,
        const tilemap::TileMap &map,
        const GridCell cell,
        const std::int32_t lift,
        const MapCamera &camera)
    {
        const auto zoom = camera.zoom();
        const auto origin = cellOrigin(cell, lift, camera);
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

                if (index >= reachable.size()
                    || !reachable[index].anyStandable
                    || reachable[index].anyReached)
                {
                    continue;
                }

                view.drawRect(
                    cellRectF(
                        cell, surfaceLift(map.at(cell)), camera),
                    kUnreachable);
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
                cellRectF(
                    *finding.at, finding.level.value_or(0), camera),
                kFinding);
        }
    }

    void drawMapSelectionOverlay(
        ViewportRenderer &view, const EditorStore &store)
    {
        const auto &sel = store.mapSelection;
        const auto lift = store.state.activeLevel;
        const auto &camera = store.camera;

        if (sel.dragging)
        {
            dashedRect(
                view,
                spanRectF(
                    cellSpanOf(sel.anchor, sel.focus),
                    lift,
                    camera),
                kSelected);
            return;
        }

        const auto span = mapSelectionSpan(store);

        if (!span.has_value())
        {
            return;
        }

        if (sel.moving)
        {
            const auto zoom = camera.zoom();
            auto rect = spanRectF(*span, lift, camera);

            rect.origin.x +=
                (static_cast<float>(sel.movePointer.column)
                 - static_cast<float>(sel.moveAnchor.column))
                * kUnitF * zoom;
            rect.origin.y +=
                (static_cast<float>(sel.movePointer.row)
                 - static_cast<float>(sel.moveAnchor.row))
                * kUnitF * zoom;
            dashedRect(view, rect, kSelected);
            return;
        }

        outlineRect(view, spanRectF(*span, lift, camera), kSelected);
    }

    void drawActiveLevelMarks(
        ViewportRenderer &view,
        const EditorState &state,
        const MapCamera &camera)
    {
        if (state.activeLevel == 0)
        {
            return;
        }

        const auto &map = state.map;
        const auto mark = chromeFor(map.header().paper).freeMark;

        for (std::uint32_t row = 0; row < map.rows(); ++row)
        {
            for (std::uint32_t column = 0; column < map.columns();
                 ++column)
            {
                const auto cell =
                    GridCell{.column = column, .row = row};

                if (map.at(cell).slabAt(state.activeLevel)
                    == nullptr)
                {
                    continue;
                }

                outlineRect(
                    view,
                    cellRectF(cell, state.activeLevel, camera),
                    mark);
            }
        }
    }

}
