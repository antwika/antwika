#include <algorithm>
#include <cstdint>

#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/ui/DoubleClick.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

#include "antwika/editor/ui/WidgetCatalog.hpp"

namespace antwika::editor
{

    void Editor::duplicateTile(
        const geometry::GridCell fromCell,
        const geometry::GridCell toCell)
    {
        const auto source =
            document.map.tilemap.getEntryAt(fromCell.column, fromCell.row);
        auto target = document.map.tilemap.getEntryAt(toCell.column, toCell.row);

        if (!target.has_value())
        {
            target = tilemap::suggestedTileFor(document.map.tilemap, toCell);

            if (target.has_value())
            {
                tilemap::putTile(document.map.tilemap, toCell, *target);
            }
        }

        if (!source.has_value() || !target.has_value()
            || source->atlas != target->atlas
            || *source == *target
            || transitionOf(document.map.transitions, *target)
                   != nullptr)
        {
            return;
        }

        copyTilePixels(*source, *target);

        atlasSheets.touch();
        rebuildWorld();
    }

    void Editor::onScrolled(const input::PointerScrolled &rolledScrolled)
    {
        pointer.wheelSteps -= rolledScrolled.vertical;

        if (rolledScrolled.vertical != 0)
        {
            for (const auto &row : getWidgetCatalog().sliderRows)
            {
                if (row.valueOf == nullptr
                    || pointer.hoveredWidget != row.widget
                    || (row.decorNeed && !isDecorLayer(chosenLayer))
                    || (row.slideGate != nullptr && !row.slideGate(*this)))
                {
                    continue;
                }

                if (row.undoNeed
                    && tick >= remesh.lastWheelNudgeTick + 60)
                {
                    pushUndo();
                }

                if (row.decorNeed)
                {
                    ensureDecor();
                }

                const auto value = static_cast<int>(row.valueOf(*this));
                const auto nudgedValue =
                    rolledScrolled.vertical > 0
                        ? std::min<int>(
                              value + 1,
                              decor::kFullFrequency)
                        : std::max<int>(value - 1, 0);

                row.slideEffect(
                    *this, static_cast<std::uint32_t>(nudgedValue));
                remesh.lastWheelNudgeTick = tick;
                remesh.afterNudge = true;

                return;
            }
        }

        if (isInspectorHovered() || isEntityListHovered())
        {
            return;
        }

        if (auto *view = viewNow();
            view != nullptr
            && view->consumeScroll(viewContextNow(), rolledScrolled))
        {
            return;
        }

        if (rolledScrolled.vertical != 0)
        {
            const auto zoomValue =
                play.playing ? play.game->getZoom() : cameraRig.view.zoom;
            const auto nextZoom = std::clamp(
                zoomValue
                    + (rolledScrolled.vertical > 0
                           ? camera::kZoomStep
                           : -camera::kZoomStep),
                camera::kMinZoom,
                camera::kMaxZoom);

            if (play.playing)
            {
                play.game->setZoom(nextZoom);
            }
            else
            {
                cameraRig.view.zoom = nextZoom;
            }
        }

        return;
    }

    bool Editor::beginSliderDrag(
        const ui::Interactions &interactions)
    {
        if (!interactions.slidChange.has_value())
        {
            return false;
        }

        for (const auto &row : getWidgetCatalog().sliderRows)
        {
            if (interactions.slidChange->sliderWidget != row.widget
                || (row.slideGate != nullptr && !row.slideGate(*this)))
            {
                continue;
            }

            if (row.undoNeed)
            {
                pushUndo();
            }

            if (row.decorNeed)
            {
                ensureDecor();
            }

            row.slideEffect(*this, interactions.slidChange->value);
            slidingWidget = row.widget;

            return true;
        }

        return false;
    }

    void Editor::endSliderDrag()
    {
        for (const auto &row : getWidgetCatalog().sliderRows)
        {
            if (slidingWidget == row.widget
                && row.settleEffect != nullptr)
            {
                row.settleEffect(*this);
            }
        }

        slidingWidget.reset();
    }

}
