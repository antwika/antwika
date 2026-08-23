#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/TilemapView.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/tile/TileShapes.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    tile::TileRules &Editor::activeRules()
    {
        return chosenLayer == map::kBaseLayer
                            ? document.map.rules
                            : document.map.decorRules;
    }

    bool Editor::isDecorLayer()
    {
        return chosenLayer != map::kBaseLayer;
    }

    void Editor::commitFloatingPatch()
    {
        if (!characterView.mark.floatingPatchBuffer.has_value()
            || !characterView.mark.selection.has_value()
            || !characterView.mark.selectedFrame.has_value())
        {
            return;
        }

        character::pasteInto(
            characterView.sheet(),
            *characterView.mark.selectedFrame / character::kCharacterFrames,
            *characterView.mark.selectedFrame % character::kCharacterFrames,
            character::selectionOrigin(*characterView.mark.selection),
            *characterView.mark.floatingPatchBuffer);

        characterView.mark.floatingPatchBuffer.reset();
        characterView.touch();
    }

    void Editor::mirrorSelection()
    {
        if (!characterView.mark.selection.has_value()
            || !characterView.mark.selectedFrame.has_value())
        {
            return;
        }

        const auto way = *characterView.mark.selectedFrame
                         / character::kCharacterFrames;
        const auto frame = *characterView.mark.selectedFrame
                           % character::kCharacterFrames;

        if (characterView.mark.floatingPatchBuffer.has_value())
        {
            characterView.mark.floatingPatchBuffer =
                character::mirroredHorizontally(
                    *characterView.mark.floatingPatchBuffer);

            return;
        }

        pushUndo();
        character::pasteInto(
            characterView.sheet(),
            way,
            frame,
            character::selectionOrigin(*characterView.mark.selection),
            character::mirroredHorizontally(
                character::copiedFrom(
                    characterView.sheet(),
                    way,
                    frame,
                    *characterView.mark.selection)));
        characterView.touch();
    }

    bool Editor::selectionIsForbidden()
    {
        return std::ranges::all_of(
            edgesIn(*selectedEdges),
            [this](const tilemap::TileEdge edge) {
                return activeRules().isForbidden(
                    *selectedTile, edge);
            });
    }

    bool Editor::selectionAllowsBoundary()
    {
        return std::ranges::all_of(
            edgesIn(*selectedEdges),
            [this](const tilemap::TileEdge edge) {
                return activeRules().allowsBoundary(
                    *selectedTile, edge);
            });
    }

    bool Editor::selectionAllows(const tilemap::Tile neighbourTile)
    {
        return std::ranges::all_of(
            edgesIn(*selectedEdges),
            [this, neighbourTile](const tilemap::TileEdge edge) {
                return activeRules().allows(
                    *selectedTile, edge, neighbourTile);
            });
    }

    void Editor::flipEdgeToggle(const EdgeToggle whichToggle)
    {
        if (!selectedTile.has_value() || !selectedEdges.has_value()
            || blockedAsVariant())
        {
            return;
        }

        pushUndo();

        if (whichToggle == EdgeToggle::Forbidden)
        {
            const auto forbidding = !selectionIsForbidden();

            for (const auto edge : edgesIn(*selectedEdges))
            {
                if (forbidding)
                {
                    activeRules().forbidAll(*selectedTile, edge);
                }
                else
                {
                    activeRules().clearRule(*selectedTile, edge);
                }
            }

            return;
        }

        const auto may = !selectionAllowsBoundary();

        for (const auto edge : edgesIn(*selectedEdges))
        {
            activeRules().setAllowsBoundary(*selectedTile, edge, may);
        }
    }

    void Editor::deriveRulesFromShapes()
    {
        if (!selectedTile.has_value())
        {
            return;
        }

        const auto kind = activeRules().kindOf(*selectedTile);
        const auto shapedRules =
            tile::rulesFromShapes(activeRules(), kind);

        if (!shapedRules.toAddRules.empty())
        {
            pushUndo();

            for (const auto &rule : shapedRules.toAddRules)
            {
                if (groupContaining(
                        document.map.familyGroups, rule.tile)
                    != nullptr)
                {
                    continue;
                }

                for (const auto tile : rule.allowedTiles)
                {
                    if (groupContaining(
                            document.map.familyGroups, tile)
                        != nullptr)
                    {
                        continue;
                    }

                    activeRules().allow(
                        rule.tile, rule.edge, tile);
                }
            }

            rebuildWorld();
        }

        logger.log(
            antwika::log::Level::Info,
            "laid " + std::to_string(shapedRules.toAddRules.size())
                + " junctions the shapes call for, and left "
                + std::to_string(shapedRules.conflictingRules.size())
                + " the shapes will not have");
    }

    gfx::RectF Editor::sheetClipRect()
    {
        return sheetRect.value_or(tilemapBounds(camera::kCanvasSize));
    }

    gfx::RectF Editor::gridRect()
    {
        return panZoomed(
            tilemapPlace(sheetClipRect(), document.map.tilemap),
            gridPanPoint,
            gridZoom);
    }

    gfx::RectF Editor::frameRect()
    {
        return canvasRect.value_or(inspectColumnBounds(camera::kCanvasSize));
    }

    std::optional<geometry::GridCell> Editor::cellUnderPointer()
    {
        return cellShownAt(
            document.map.tilemap, gridRect(), sheetClipRect(),
            pointer.pointerOnCanvas);
    }

    void Editor::drawColorPicker()
    {
        if (!inkPicker.editingInk.has_value())
        {
            return;
        }

        viewportRenderer.drawRect(
            pickerPlace(camera::kCanvasSize),
            kGridLineColor);

        for (std::size_t cursorY = 0; cursorY < kPickerBands;
             ++cursorY)
        {
            for (std::size_t column = 0;
                 column < kPickerBands;
                 ++column)
            {
                viewportRenderer.drawRect(
                    bandPlace(camera::kCanvasSize, column, cursorY),
                    colorOf(bandHsv(inkPicker.pickerHsv, column, cursorY)));
            }

            viewportRenderer.drawRect(
                hueBandPlace(camera::kCanvasSize, cursorY),
                colorOf(
                    Hsv{
                        .hue = hueBand(cursorY),
                        .saturation = 1.0F,
                        .value = 1.0F}));
        }

        const auto mark = fieldCursorPos(
            camera::kCanvasSize,
            inkPicker.pickerHsv);

        viewportRenderer.drawLine(
            {mark.x - kCursorArmLength, mark.y},
            {mark.x + kCursorArmLength, mark.y},
            kTextColor);
        viewportRenderer.drawLine(
            {mark.x, mark.y - kCursorArmLength},
            {mark.x, mark.y + kCursorArmLength},
            kTextColor);

        const auto strip = huePlace(camera::kCanvasSize);
        const auto cursorY = hueCursorPos(
            camera::kCanvasSize,
            inkPicker.pickerHsv);

        viewportRenderer.drawLine(
            {strip.originPoint.x - kCursorArmLength, cursorY},
            {strip.originPoint.x + strip.size.width + kCursorArmLength,
             cursorY},
            kTextColor);

        for (const auto bar :
             outlineRects(pickerPlace(camera::kCanvasSize), kBorderThick))
        {
            viewportRenderer.drawRect(bar, kTextColor);
        }
    }

    std::string Editor::statusText()
    {
        const auto hint = hintFor(pointer.hoveredWidget);

        if (!hint.empty())
        {
            return std::string(hint);
        }

        if (activeView == map::View::Character)
        {
            return inkPicker.editingInk.has_value()
                       ? "drag in the picker to mix a "
                         "colour - ok keeps it, cancel "
                         "leaves it, delete takes it away"
                   : characterView.mark.selection.has_value()
                       ? "drag the mark to carry the pixels "
                         "- rmb lays them down - h flips "
                         "them - ctrl c, x, v copy, cut, "
                         "paste"
                       : "3 character - drag draws - rmb "
                         "rubs out - m marks a rectangle out "
                         "- shift marks with any tool";
        }

        if (activeView == map::View::Icons)
        {
            return "4 icons - click an icon to take it up - "
                   "lmb paints - rmb rubs out - saved with "
                   "the map";
        }

        if (activeView == map::View::Plan)
        {
            return plan.picked()
                       ? "drag a card to carry it - write its "
                         "title and what it means on the right "
                         "- escape leaves a field"
                       : "5 plan - click a card to write in it "
                         "- drag one to carry it between "
                         "columns - ctrl s keeps the board";
        }

        if (activeView == map::View::Atlases)
        {
            return inkPicker.editingInk.has_value()
                       ? "drag in the picker to mix a "
                         "colour - ok keeps it, cancel "
                         "leaves it, delete takes it away"
                   : !selectedEdges.has_value()
                           && selectedTile.has_value()
                       ? "shift n, w, r give this tile a kind "
                         "- the buttons give it a facing"
                   : !selectedEdges.has_value()
                       ? "click a tile to look at it, drag to "
                         "swap, del takes it out"
                   : selectedTile.has_value()
                           && selectionIsForbidden()
                       ? "this edge meets nothing - x opens it"
                   : selectedTile.has_value()
                           && selectionAllowsBoundary()
                       ? "may meet these, and the rim - r "
                         "rims, x shuts"
                       : "may meet these, not the rim - r "
                         "rims, x shuts";
        }

        return std::string(
                   settings.tool == map::Tool::Brush
                         ? "1 world - lmb adds - rmb takes - "
                         "f5 plays"
                   : settings.tool == map::Tool::Lamp
                       ? "1 world - lmb sets a lamp of the "
                         "ink chosen - rmb takes"
                   : settings.tool == map::Tool::Start
                       ? "1 world - lmb sets the start cube "
                         "- rmb takes it"
                   : settings.tool == map::Tool::Exit
                       ? "1 world - lmb sets the exit cube "
                         "- rmb takes it"
                   : settings.tool == map::Tool::Stamp
                       ? "1 world - drag copies cubes - lmb "
                         "sets them down - rmb drops them"
                   : settings.tool == map::Tool::Figure
                       ? "1 world - lmb stands the chosen "
                         "figure here, again adds a stop - "
                         "rmb takes it away"
                   : settings.tool == map::Tool::PressurePlate
                       ? "1 world - lmb sets the plate, then "
                         "picks the cubes it sways - rmb "
                         "takes it"
                   : settings.tool == map::Tool::Food
                       ? "1 world - lmb lays food to pick up "
                         "- rmb takes it"
                   : settings.tool == map::Tool::Water
                       ? "1 world - lmb lays water to pick up "
                         "- rmb takes it"
                   : settings.tool == map::Tool::Eraser
                       ? "1 world - lmb clears cubes - drag "
                         "sweeps them away"
                       : "1 world - lmb picks a tile - rmb "
                         "takes")
               + " - wheel zooms - g - c "
               + std::string(
                   cornerJoining
                           == solver::CornerSeams::Included
                            ? "on"
                            : "off")
               + " - level " + std::to_string(editLevel);
    }

    void Editor::recordFrameWork(
        const std::chrono::time_point<std::chrono::system_clock> startedAt)
    {
        meters.workRate.record(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                clockSource.currentTime() - startedAt));
    }

}
