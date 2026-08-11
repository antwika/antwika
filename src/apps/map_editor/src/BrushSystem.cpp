#include "antwika/map_editor/BrushSystem.hpp"

#include <cstddef>
#include <optional>

#include <antwika/geometry/Grid.hpp>

#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    namespace
    {
        [[nodiscard]] bool outsideMap(
            const EditorState &state, const SignedCell cell) noexcept
        {
            return cell.column < 0 || cell.row < 0
                   || cell.column >= static_cast<std::int32_t>(
                          state.map.columns())
                   || cell.row >= static_cast<std::int32_t>(
                          state.map.rows());
        }

        [[nodiscard]] std::optional<std::size_t> markerIndexAt(
            const World &world, const geometry::GridCell cell)
        {
            for (const auto entity : world.view<Marker, CellRef>())
            {
                const auto &at = world.get<CellRef>(entity);

                if (at.column != cell.column || at.row != cell.row)
                {
                    continue;
                }

                return world.get<Marker>(entity).index;
            }

            return std::nullopt;
        }
    }

    BrushSystem::BrushSystem(EditorStore &store) : store(store)
    {
    }

    void BrushSystem::update(World &world, antwika::time::Tick)
    {
        auto &state = store.state;

        if (store.view == EditorView::Tiles)
        {
            for (const auto &gesture : store.input.sheetGestures)
            {
                applyTilesetGesture(store, gesture);
            }

            return;
        }

        if (store.view == EditorView::Characters)
        {
            for (const auto &gesture : store.input.sheetGestures)
            {
                if (store.characters.tool
                    == CharacterTool::Select)
                {
                    applyCharSelectGesture(store, gesture);
                }
                else
                {
                    applySheetGesture(store, gesture);
                }
            }

            return;
        }

        if (store.mapTool == MapTool::Select)
        {
            for (const auto &gesture : store.input.gestures)
            {
                applyMapSelectGesture(store, gesture);
            }

            return;
        }

        for (const auto &gesture : store.input.gestures)
        {
            if (gesture.erase)
            {
                if (gesture.kind != GestureKind::Release
                    && !outsideMap(state, gesture.signedCell))
                {
                    state.hovered = gesture.cell;
                    eraseSlabHovered(state);
                }

                continue;
            }

            if (gesture.kind == GestureKind::Release)
            {
                state.painting = false;
                continue;
            }

            if (gesture.kind == GestureKind::Move)
            {
                if (!state.painting)
                {
                    continue;
                }

                if (outsideMap(state, gesture.signedCell))
                {
                    paintBeyond(gesture.signedCell);
                    continue;
                }

                state.hovered = gesture.cell;
                paintHovered(state);
                continue;
            }

            if (outsideMap(state, gesture.signedCell))
            {
                store.ui.selected.reset();
                state.painting = true;
                paintBeyond(gesture.signedCell);
                continue;
            }

            const auto found = markerIndexAt(world, gesture.cell);

            if (found.has_value())
            {
                store.ui.selected = found;
                loadEntityBuffers(store);

                if (widgets::isField(store.ui.focus))
                {
                    store.ui.focus = ui::kNoWidget;
                }

                continue;
            }

            store.ui.selected.reset();
            state.hovered = gesture.cell;
            state.painting = true;
            paintHovered(state);
        }
    }

    void BrushSystem::paintBeyond(const SignedCell target)
    {
        auto &state = store.state;
        const auto landed = extendMapFor(state, target);

        if (!landed.has_value())
        {
            return;
        }

        if (landed->west > 0 || landed->north > 0)
        {
            const auto zoom = store.camera.zoom();

            store.camera.panX -=
                static_cast<float>(landed->west) * 16.0F * zoom;
            store.camera.panY -=
                static_cast<float>(landed->north) * 16.0F * zoom;
        }

        state.hovered = landed->landed;
        state.hoveredBeyond.reset();
        paintExtended(state);
    }

}
