#include <initializer_list>

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

    void Editor::flipEdgeToggle(const EdgeToggle whichToggle)
    {
        if (!stroke.selectedTile.has_value() || !stroke.selectedEdges.has_value()
            || blockedAsVariant())
        {
            return;
        }

        pushUndo();

        if (whichToggle == EdgeToggle::Forbidden)
        {
            const auto forbidding = !stroke.isForbidden(getActiveRules(document.map, chosenLayer));

            for (const auto edge : edgesIn(*stroke.selectedEdges))
            {
                if (forbidding)
                {
                    getActiveRules(document.map, chosenLayer).forbidAll(*stroke.selectedTile, edge);
                }
                else
                {
                    getActiveRules(document.map, chosenLayer).clearRule(*stroke.selectedTile, edge);
                }
            }

            return;
        }

        const auto may = !stroke.allowsBoundary(getActiveRules(document.map, chosenLayer));

        for (const auto edge : edgesIn(*stroke.selectedEdges))
        {
            getActiveRules(
                document.map, chosenLayer).setAllowsBoundary(*stroke.selectedTile, edge, may);
        }
    }

    void Editor::deriveRulesFromShapes()
    {
        if (!stroke.selectedTile.has_value())
        {
            return;
        }

        const auto kind = getActiveRules(document.map, chosenLayer).kindOf(*stroke.selectedTile);
        const auto shapedRules =
            tile::getRulesFromShapes(getActiveRules(document.map, chosenLayer), kind);

        if (!shapedRules.toAddRules.empty())
        {
            pushUndo();

            for (const auto &rule : shapedRules.toAddRules)
            {
                if (getGroupContaining(
                        document.map.familyGroups, rule.tile)
                    != nullptr)
                {
                    continue;
                }

                for (const auto tile : rule.allowedTiles)
                {
                    if (getGroupContaining(
                            document.map.familyGroups, tile)
                        != nullptr)
                    {
                        continue;
                    }

                    getActiveRules(document.map, chosenLayer).allow(
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

    std::optional<geometry::GridCell> Editor::cellUnderPointer()
    {
        return sheetView.getCellUnder(
            document.map.tilemap, pointer.pointerOnCanvas);
    }

    void Editor::drawColorPicker()
    {
        if (!inkPicker.editingInk.has_value())
        {
            return;
        }

        viewportRenderer.drawRect(
            getPickerPlace(camera::kCanvasSize),
            kGridLineColor);

        for (std::size_t cursorY = 0; cursorY < kPickerBands;
             ++cursorY)
        {
            for (std::size_t column = 0;
                 column < kPickerBands;
                 ++column)
            {
                viewportRenderer.drawRect(
                    getBandPlace(camera::kCanvasSize, column, cursorY),
                    colorOf(getBandHsv(inkPicker.pickerHsv, column, cursorY)));
            }

            viewportRenderer.drawRect(
                getHueBandPlace(camera::kCanvasSize, cursorY),
                colorOf(
                    Hsv{
                        .hue = getHueBand(cursorY),
                        .saturation = 1.0F,
                        .value = 1.0F}));
        }

        const auto mark = getFieldCursorPos(
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

        const auto strip = getHuePlace(camera::kCanvasSize);
        const auto cursorY = getHueCursorPos(
            camera::kCanvasSize,
            inkPicker.pickerHsv);

        viewportRenderer.drawLine(
            {strip.originPoint.x - kCursorArmLength, cursorY},
            {strip.originPoint.x + strip.size.width + kCursorArmLength,
             cursorY},
            kTextColor);

        for (const auto bar :
             getOutlineRects(getPickerPlace(camera::kCanvasSize), kBorderThick))
        {
            viewportRenderer.drawRect(bar, kTextColor);
        }
    }

    ViewContext Editor::viewContextNow() noexcept
    {
        return ViewContext{
            .document = document,
            .play = play,
            .cameraRig = cameraRig,
            .caption = caption,
            .meters = meters,
            .clockSource = clockSource,
            .workbench =
                Workbench{
                    .preferences = preferences,
                    .stroke = stroke,
                    .sheetView = sheetView,
                    .pointer = pointer,
                    .inkPicker = inkPicker,
                    .keyBench = keyBench,
                    .focusedField = focusedField,
                    .chosenLayer = chosenLayer,
                    .assignMode = assignMode,
                    .transition = transition},
            .render =
                WorldRender{
                    .viewportRenderer = viewportRenderer,
                    .atlasSheets = atlasSheets,
                    .worldMeshes = worldMeshes,
                    .worldShader = worldShader,
                    .sprites = sprites,
                    .scenePass = scenePass,
                    .lightPasses = lightPasses,
                    .rosterSkins = rosterSkins},
            .editSteps = *this,
            .notices = *this,
            .shownView = viewChoice.activeView,
            .heldModifiers = getHeldModifiers(),
            .tick = tick};
    }

    IEditorView *Editor::viewNow() noexcept
    {
        for (IEditorView *view :
             std::initializer_list<IEditorView *>{
                 &iconsView,
                 &plan,
                 &characterView,
                 &atlasView,
                 &worldView})
        {
            if (view->claims(viewChoice.activeView, play.playing))
            {
                return view;
            }
        }

        return nullptr;
    }

    bool Editor::isWorldShown() const noexcept
    {
        return worldView.claims(viewChoice.activeView, play.playing);
    }

    std::string Editor::statusText()
    {
        const auto hint = hintFor(pointer.hoveredWidget);

        if (!hint.empty())
        {
            return std::string(hint);
        }

        if (inkPicker.editingInk.has_value())
        {
            return "drag in the picker to mix a "
                   "colour - ok keeps it, cancel "
                   "leaves it, delete takes it away";
        }

        auto *view = viewNow();

        return view != nullptr ? view->getStatusText(viewContextNow())
                               : std::string{};
    }

    void Editor::recordFrameWork(
        const std::chrono::time_point<std::chrono::system_clock> startedAt)
    {
        meters.workRate.record(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                clockSource.getCurrentTime() - startedAt));
    }

}
