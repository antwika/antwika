#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/CharacterView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/IconSheet.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::pressedOnSheets(
        const input::PointerButtonPressed &downPressed)
    {
        if ((downPressed.button == input::MouseButton::Left
             || downPressed.button == input::MouseButton::Right)
            && activeView == map::View::Icons)
        {
            const auto projectToScreen =
                viewportRenderer.getViewport().toCanvas(
                    antwika::gfx::Point{
                        .x = downPressed.position.x,
                        .y = downPressed.position.y});

            pointer.pointerOnCanvas = antwika::gfx::PointF{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)};

            const auto count = iconsView.getCount();
            const auto chosenCell = antwika::editor::iconCellAt(
                camera::kCanvasSize, count, pointer.pointerOnCanvas);

            if (chosenCell.has_value())
            {
                iconsView.pick(
                    downPressed.button == input::MouseButton::Right
                                        ? std::nullopt
                                        : std::optional{*chosenCell});

                return;
            }

            if (!iconsView.getPickedIndex().has_value()
                || *iconsView.getPickedIndex() >= count)
            {
                return;
            }

            const auto pixel = antwika::editor::iconPixelAt(
                antwika::editor::getEditedIconRect(camera::kCanvasSize),
                pointer.pointerOnCanvas);

            if (!pixel.has_value())
            {
                return;
            }

            strokeErases = downPressed.button == input::MouseButton::Right;
            iconsView.paint(viewportRenderer, *pixel, strokeErases);
            document.markDirty();
            brushAtCell = pixel;
            strokeActive = true;

            return;
        }

        if ((downPressed.button == input::MouseButton::Left
             || downPressed.button == input::MouseButton::Right)
            && activeView == map::View::Character)
        {
            const auto projectToScreen =
                viewportRenderer.getViewport().toCanvas(
                    antwika::gfx::Point{
                        .x = downPressed.position.x,
                        .y = downPressed.position.y});

            pointer.pointerOnCanvas = antwika::gfx::PointF{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)};

            const auto chosenCell =
                characterAt(camera::kCanvasSize, pointer.pointerOnCanvas);

            if (chosenCell.has_value())
            {
                commitFloatingPatch();
                characterView.mark.selection.reset();

                if (downPressed.button
                    == input::MouseButton::Right)
                {
                    characterView.mark.selectedFrame.reset();
                }
                else
                {
                    characterView.mark.selectedFrame = *chosenCell;
                }

                return;
            }

            if (!characterView.mark.selectedFrame.has_value())
            {
                return;
            }

            const auto pixel = character::characterPixelAt(
                getCharacterCanvasRect(camera::kCanvasSize),
                pointer.pointerOnCanvas);

            if (!pixel.has_value())
            {
                return;
            }

            if (downPressed.button == input::MouseButton::Right
                && characterView.mark.selection.has_value())
            {
                commitFloatingPatch();
                characterView.mark.selection.reset();

                return;
            }

            if (downPressed.button == input::MouseButton::Left
                && (getHeldModifiers().shift
                    || settings.paint == map::Paint::Select))
            {
                if (characterView.mark.selection.has_value()
                    && character::isSelectionContains(
                        *characterView.mark.selection, *pixel))
                {
                    characterView.mark.draggingPatch = true;
                    characterView.mark.grabbedMarkSelection =
                        characterView.mark.selection;
                    characterView.mark.grabbedAtCell = pixel;

                    if (!characterView.mark.floatingPatchBuffer.has_value())
                    {
                        characterView.mark.floatingPatchBuffer =
                            character::cutFrom(
                                characterView.getSheet(),
                                *characterView.mark.selectedFrame
                                    / character::kCharacterFrames,
                                *characterView.mark.selectedFrame
                                    % character::kCharacterFrames,
                                *characterView.mark.selection);
                        characterView.touch();
                    }

                    return;
                }

                commitFloatingPatch();
                characterView.mark.selecting = true;
                characterView.mark.selection = character::PixelSelection{
                    .fromCell = *pixel, .toCell = *pixel};

                return;
            }

            commitFloatingPatch();
            characterView.mark.selection.reset();
            strokeErases =
                downPressed.button == input::MouseButton::Right;
            pushUndo();

            const auto color = character::getCharacterPaletteColor(
                document.map.paletteColors,
                strokeErases ? character::kTransparentInk
                             : inkPicker.activeInk);

            if (settings.paint == map::Paint::Fill && !strokeErases)
            {
                character::paintCharacterFill(
                    characterView.getSheet(),
                    *characterView.mark.selectedFrame
                        / character::kCharacterFrames,
                    *characterView.mark.selectedFrame
                        % character::kCharacterFrames,
                    *pixel,
                    color);
            }
            else
            {
                character::paintCharacter(
                    characterView.getSheet(),
                    *characterView.mark.selectedFrame
                        / character::kCharacterFrames,
                    *characterView.mark.selectedFrame
                        % character::kCharacterFrames,
                    *pixel,
                    color);
                brushAtCell = pixel;
                strokeActive = true;
            }

            characterView.touch();

            return;
        }

        if (downPressed.button == input::MouseButton::Right
            && activeView == map::View::Atlases
            && !inkPicker.editingInk.has_value())
        {
            const auto canvasPoint = viewportRenderer.getViewport().toCanvas(
                antwika::gfx::Point{
                    .x = downPressed.position.x,
                    .y = downPressed.position.y});

            pointer.pointerOnCanvas = antwika::gfx::PointF{
                static_cast<float>(canvasPoint.x),
                static_cast<float>(canvasPoint.y)};

            if (selectedTile.has_value())
            {
                const auto editedTileValue = editedTile();
                const auto pixel = tile::pixelAt(
                    editedTileValue,
                    getInspectedTileRect(frameRect(), editedTileValue),
                    pointer.pointerOnCanvas);

                if (pixel.has_value())
                {
                    pushUndo();
                    tile::paint(
                        atlasSheets.sheet(editedTileValue.atlas),
                        editedTileValue,
                        *pixel,
                        antwika::gfx::Color{.alpha = 0});
                    brushAtCell = pixel;
                    strokeActive = true;
                    strokeErases = true;
                    atlasSheets.touch();

                    return;
                }
            }

            selectedTile.reset();
            selectedEdges.reset();
            lineFromCell.reset();
            clearAssignModes();
            transitFromTile.reset();
            transitToTile.reset();

            return;
        }

        if (downPressed.button != input::MouseButton::Left
            || activeView != map::View::Atlases)
        {
            return;
        }

        const auto projectToScreen = viewportRenderer.getViewport().toCanvas(
            antwika::gfx::Point{
                .x = downPressed.position.x, .y = downPressed.position.y});

        pointer.pointerOnCanvas = antwika::gfx::PointF{
            static_cast<float>(projectToScreen.x),
            static_cast<float>(projectToScreen.y)};

        if (paintedOnAtlasPixel())
        {
            return;
        }

        dragFromPoint = pointer.pointerOnCanvas;
        dragFromCell = cellUnderPointer();
    }

    bool Editor::paintedOnAtlasPixel()
    {
        if (!selectedTile.has_value())
        {
            return false;
        }

        const auto editedTileValue = editedTile();
        const auto pixel = tile::pixelAt(
            editedTileValue,
            getInspectedTileRect(frameRect(), editedTileValue),
            pointer.pointerOnCanvas);

        if (!pixel.has_value())
        {
            return false;
        }

        if (blockedAsTransitionSlot())
        {
            return true;
        }

        if (settings.paint == map::Paint::Line
            || settings.paint == map::Paint::Rect
            || settings.paint == map::Paint::Circle)
        {
            lineFromCell = pixel;

            return true;
        }

        auto &sheet = atlasSheets.sheet(editedTileValue.atlas);

        pushUndo();

        if (settings.paint == map::Paint::Fill)
        {
            tile::paintFill(
                sheet,
                editedTileValue,
                *pixel,
                document.map.paletteColors.at(inkPicker.activeInk));
        }
        else
        {
            tile::paint(
                sheet,
                editedTileValue,
                *pixel,
                document.map.paletteColors.at(inkPicker.activeInk));
            brushAtCell = pixel;
            strokeActive = true;
        }

        atlasSheets.touch();

        return true;
    }

}
