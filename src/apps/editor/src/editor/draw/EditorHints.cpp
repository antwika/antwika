#include <algorithm>

#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/LayerWidgets.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/ui/HoverHint.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/text/TextLayout.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/ui/WidgetCatalog.hpp"

namespace antwika::editor
{

    void Editor::drawToolHint(const ui::Frame &frame)
    {
        if (!isTooltipDue(pointer.hoverTracker, tick)
            || pointer.hoverTracker.widget != pointer.hoveredWidget
            || !widget_catalog::isOnToolPanel(
                getWidgetCatalog(), *this, pointer.hoveredWidget))
        {
            return;
        }

        const auto hint = hintFor(pointer.hoveredWidget);
        const auto room = frame.rects.getWidgetRect(pointer.hoveredWidget);

        if (hint.empty() || !room.has_value())
        {
            return;
        }

        const auto pad = static_cast<float>(2 * kUiScale);
        const auto hintSize = text::getTextSize(
            hint, gfx::TextScale{.multiplier = kUiScale});
        const auto width =
            static_cast<float>(hintSize.width) + (pad * 2.0F);
        const auto height =
            static_cast<float>(hintSize.height) + (pad * 2.0F);
        const auto window = viewportRenderer.getWindowSize();

        const auto left = std::min(
            static_cast<float>(
                room->originPoint.x + room->size.width)
                + pad,
            static_cast<float>(window.width) - width);
        const auto top = std::min(
            static_cast<float>(room->originPoint.y),
            static_cast<float>(window.height) - height);

        auto &innerRenderer = viewportRenderer.innerRenderer();

        innerRenderer.drawRect(
            gfx::RectF({left, top}, {width, height}), kTitleBarColor);
        innerRenderer.drawText(
            {left + pad, top + pad},
            hint,
            gfx::TextScale{.multiplier = kUiScale},
            kTextColor);
    }

    namespace
    {

        [[nodiscard]] std::string getTileLabel(
            const tilemap::Tile tile, const std::string_view tileName = "tile")
        {
            return std::string(
                       tile.atlas == tilemap::Atlas::Floor ? "Floor"
                                   : "Wall")
                   + " " + std::string(tileName) + " #"
                   + std::to_string(tile.index);
        }

    }

    void Editor::updateCanvasHover(const ui::Frame &frame)
    {
        auto tileCell = std::optional<geometry::GridCell>{};
        auto face = std::optional<std::size_t>{};

        const auto clear =
            !play.playing && !dialogs.quitConfirmOpen && !keyBench.panelShown
            && !fileChooser.fileDialog.has_value()
            && !inkPanel.inkPicker.editingInk.has_value()
            && !frame.interactions.pointerOverUi;

        if (clear && viewChoice.activeView == View::Atlases)
        {
            const auto cell = cellUnderPointer();

            if (cell.has_value()
                && document.map.tilemap.getEntryAt(cell->column, cell->row)
                       .has_value())
            {
                tileCell = cell;
            }
        }

        if (clear && isWorldShown()
            && preferences.tool == Tool::Picker)
        {
            face = voxelmap::getFacePicked(
                visibleCells(),
                worldMeshes.getFaces(),
                getWorldCamera(play, cameraRig),
                getWorldRotation(play),
                camera::kCanvasSize,
                pointer.pointerOnCanvas);
        }

        if (tileCell != pointer.canvasRest.tileCell
            || face != pointer.canvasRest.face)
        {
            pointer.canvasRest = CanvasRest{
                .tileCell = tileCell,
                .face = face,
                .sinceTick = tick};
        }
    }

    void Editor::drawCanvasHint()
    {
        if (tick - pointer.canvasRest.sinceTick
                < ui::kTooltipDelayFrames
            || !pointer.pointerInWindow.has_value())
        {
            return;
        }

        auto hint = std::string();

        if (pointer.canvasRest.tileCell.has_value())
        {
            const auto tile = document.map.tilemap.getEntryAt(
                pointer.canvasRest.tileCell->column,
                pointer.canvasRest.tileCell->row);

            if (!tile.has_value())
            {
                return;
            }

            hint = getTileLabel(*tile);
        }
        else if (pointer.canvasRest.face.has_value()
                 && *pointer.canvasRest.face < worldMeshes.getFaces().size())
        {
            const auto face = *pointer.canvasRest.face;
            const auto tile =
                face < worldMeshes.getDrawnAs().size()
                    ? worldMeshes.getDrawnAs().at(face)
                    : voxelmap::getFaceTile(worldMeshes.getFaces().at(face));

            hint = "Voxel face #" + std::to_string(face)
                   + ", " + getTileLabel(tile);

            for (const auto &[layer, layerDecor] : worldMeshes.getDecorLayers())
            {
                const auto foundPlacement = layerDecor.find(face);

                if (foundPlacement != layerDecor.end())
                {
                    hint += ", "
                            + getTileLabel(foundPlacement->second, "decor")
                            + " (" + map::getLayerLabel(layer) + ")";
                }
            }
        }
        else
        {
            return;
        }

        const auto pad = static_cast<float>(2 * kUiScale);
        const auto hintSize = text::getTextSize(
            hint, gfx::TextScale{.multiplier = kUiScale});
        const auto width =
            static_cast<float>(hintSize.width) + (pad * 2.0F);
        const auto height =
            static_cast<float>(hintSize.height) + (pad * 2.0F);
        const auto window = viewportRenderer.getWindowSize();

        const auto left = std::clamp(
            static_cast<float>(pointer.pointerInWindow->x) + (pad * 2.0F),
            0.0F,
            static_cast<float>(window.width) - width);
        const auto top = std::clamp(
            static_cast<float>(pointer.pointerInWindow->y) + (pad * 2.0F),
            0.0F,
            static_cast<float>(window.height) - height);

        auto &innerRenderer = viewportRenderer.innerRenderer();

        innerRenderer.drawRect(
            gfx::RectF({left, top}, {width, height}), kTitleBarColor);
        innerRenderer.drawText(
            {left + pad, top + pad},
            hint,
            gfx::TextScale{.multiplier = kUiScale},
            kTextColor);
    }

    std::string_view Editor::hintFor(const widget::WidgetId whichWidget) const
    {
        const auto &catalog = getWidgetCatalog();

        for (const auto &row : catalog.soloRows)
        {
            if (whichWidget == row.widget)
            {
                return row.hint;
            }
        }

        for (const auto &family : catalog.familyRows)
        {
            const auto placeEnd = widget_catalog::placeEndIn(family, *this);

            for (auto place = family.firstPlace; place < placeEnd; ++place)
            {
                if (whichWidget == family.widgetAt(place))
                {
                    return widget_catalog::hintIn(family, place);
                }
            }
        }

        return "";
    }

}
