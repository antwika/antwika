#include "antwika/editor/ui/CharacterSheetView.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include <antwika/render/Checkerboard.hpp>
#include "antwika/editor/ui/CharacterView.hpp"
#include "antwika/editor/ui/EditorLook.hpp"

namespace antwika::editor
{

    void CharacterSheetView::open(
        gfx::ViewportRenderer &viewportRenderer, gfx::Bitmap sheetBitmap)
    {
        editedSheet = std::move(sheetBitmap);
        sheetTexture = viewportRenderer.createTexture(editedSheet);
        sheetDirty = false;

        if (!sheetCheckerTexture)
        {
            sheetCheckerTexture = viewportRenderer.createTexture(
                render::getCheckerboardBitmap(character::kCharacterCellSize, 4));
        }
    }

    void CharacterSheetView::takeSkins(
        gfx::ViewportRenderer &viewportRenderer,
        render::CharacterSkins &rosterSkins,
        std::vector<gfx::Bitmap> skinBitmaps)
    {
        rosterSkins.take(viewportRenderer, std::move(skinBitmaps));
        editingAt = std::min(editingAt, rosterSkins.getSize() - 1);
        editedSheet = rosterSkins.getSheets().at(editingAt);
        sheetDirty = true;
    }

    
    std::vector<gfx::Bitmap> CharacterSheetView::getSkinsAsDrawn(
        const render::CharacterSkins &rosterSkins) const
    {
        auto sheets = rosterSkins.getSheets();

        if (editingAt < sheets.size())
        {
            sheets.at(editingAt) = editedSheet;
        }

        return sheets;
    }

    void CharacterSheetView::keepEdits(
        gfx::ViewportRenderer &viewportRenderer,
        render::CharacterSkins &rosterSkins)
    {
        rosterSkins.lay(viewportRenderer, editingAt, editedSheet);
    }

    std::size_t CharacterSheetView::getEditing() const noexcept
    {
        return editingAt;
    }

    void CharacterSheetView::editFirst() noexcept
    {
        editingAt = 0;
    }

    void CharacterSheetView::switchTo(
        gfx::ViewportRenderer &viewportRenderer,
        render::CharacterSkins &rosterSkins,
        const std::size_t skinIndex)
    {
        if (editingAt == skinIndex || skinIndex >= rosterSkins.getSize())
        {
            return;
        }

        rosterSkins.lay(viewportRenderer, editingAt, editedSheet);
        editingAt = skinIndex;
        editedSheet = rosterSkins.getSheets().at(skinIndex);
        sheetDirty = true;
    }

    void CharacterSheetView::repaint(
        gfx::ViewportRenderer &viewportRenderer,
        render::CharacterSkins &rosterSkins,
        const std::size_t skinIndex,
        gfx::Bitmap skinBitmap)
    {
        rosterSkins.lay(viewportRenderer, skinIndex, std::move(skinBitmap));
    }

    gfx::Bitmap &CharacterSheetView::getSheet() noexcept
    {
        return editedSheet;
    }

    const gfx::Bitmap &CharacterSheetView::getSheet() const noexcept
    {
        return editedSheet;
    }

    void CharacterSheetView::touch() noexcept
    {
        sheetDirty = true;
    }

    void CharacterSheetView::refresh(gfx::ViewportRenderer &viewportRenderer)
    {
        if (!sheetDirty)
        {
            return;
        }

        sheetTexture = viewportRenderer.createTexture(editedSheet);
        sheetDirty = false;
    }

    gfx::ITexture *CharacterSheetView::getTexture() const noexcept
    {
        return sheetTexture.get();
    }

    
    gfx::ITexture *CharacterSheetView::getChecker() const noexcept
    {
        return sheetCheckerTexture.get();
    }

    void CharacterSheetView::drawSheet(
        gfx::ViewportRenderer &viewportRenderer) const
    {
        for (std::size_t way = 0; way < character::kCharacterWays;
             ++way)
        {
            for (std::size_t frame = 0;
                 frame < character::kCharacterFrames;
                 ++frame)
            {
                const auto chosenFrame =
                    mark.selectedFrame
                    == (way * character::kCharacterFrames) + frame;

                const auto place =
                    getCharacterPlace(camera::kCanvasSize, way, frame);

                viewportRenderer.drawTexture(
                    *sheetCheckerTexture,
                    antwika::gfx::RectF(
                        {0.0F, 0.0F},
                        {static_cast<float>(
                             character::
                                 kCharacterCellSize.width),
                         static_cast<float>(
                             character::
                                 kCharacterCellSize
                                     .height)}),
                    place,
                    kWhiteColor);
                viewportRenderer.drawTexture(
                    *sheetTexture,
                    character::getCharacterSource(way, frame),
                    place,
                    chosenFrame ? kWhiteColor : kDisabledTintColor);

                drawOutline(
                    viewportRenderer,
                    place,
                    chosenFrame ? kSelectionAccentColor : kGridLineColor);
            }
        }

        const auto drawnAt =
            getCharacterCanvasRect(camera::kCanvasSize);

        if (mark.selectedFrame.has_value())
        {
            viewportRenderer.drawRect(drawnAt, kPanelColor);
            viewportRenderer.drawTexture(
                *sheetCheckerTexture,
                antwika::gfx::RectF(
                    {0.0F, 0.0F},
                    {static_cast<float>(
                         character::kCharacterCellSize
                             .width),
                     static_cast<float>(
                         character::kCharacterCellSize
                             .height)}),
                drawnAt,
                kWhiteColor);

            for (std::uint32_t row = 0;
                 row < character::kCharacterCellSize.height;
                 ++row)
            {
                for (std::uint32_t column = 0;
                     column
                     < character::kCharacterCellSize.width;
                     ++column)
                {
                    const antwika::geometry::GridCell pixelCell{
                        column, row};

                    viewportRenderer.drawRect(
                        character::getCharacterPixelPlace(drawnAt, pixelCell),
                        character::getCharacterPixelColor(
                            editedSheet,
                            *mark.selectedFrame
                                / character::kCharacterFrames,
                            *mark.selectedFrame
                                % character::kCharacterFrames,
                            pixelCell));
                }
            }

            if (mark.floatingPatchBuffer.has_value()
                && mark.selection.has_value())
            {
                const auto corner =
                    character::getSelectionOrigin(*mark.selection);

                for (std::uint32_t row = 0;
                     row < mark.floatingPatchBuffer->size.height;
                     ++row)
                {
                    for (std::uint32_t column = 0;
                         column < mark.floatingPatchBuffer->size.width;
                         ++column)
                    {
                        viewportRenderer.drawRect(
                            character::getCharacterPixelPlace(
                                drawnAt,
                                antwika::geometry::GridCell{
                                    corner.column + column,
                                    corner.row + row}),
                            mark.floatingPatchBuffer->pixelColors.at(
                                (static_cast<std::size_t>(row)
                                 * mark.floatingPatchBuffer->size.width)
                                + column));
                    }
                }
            }

            if (mark.selection.has_value())
            {
                const auto place = character::
                    getSelectionRect(drawnAt, *mark.selection);

                drawOutline(
                    viewportRenderer, place, kSelectionAccentColor);
            }

            viewportRenderer.drawText(
                antwika::gfx::PointF{
                    drawnAt.originPoint.x,
                    drawnAt.originPoint.y + drawnAt.size.height
                        + 4.0F},
                std::string(
                    character::getDirectionName(
                        *mark.selectedFrame
                            / character::kCharacterFrames)),
                1,
                kTextColor);
        }
    }


    bool CharacterSheetView::claims(
        const map::View shownView, const bool playing) const noexcept
    {
        return !playing && shownView == map::View::Character;
    }

    std::string CharacterSheetView::getStatusText(const ViewContext &) const
    {
        return mark.selection.has_value()
                   ? "drag the mark to carry the pixels "
                     "- rmb lays them down - h flips "
                     "them - ctrl c, x, v copy, cut, "
                     "paste"
                   : "3 character - drag draws - rmb "
                     "rubs out - m marks a rectangle out "
                     "- shift marks with any tool";
    }

    void CharacterSheetView::draw(
        const ViewContext &viewContext, const ui::Frame &)
    {
        drawSheet(viewContext.render.viewportRenderer);
    }


    void CharacterSheetView::commitFloatingPatch()
    {
        if (!mark.floatingPatchBuffer.has_value()
            || !mark.selection.has_value()
            || !mark.selectedFrame.has_value())
        {
            return;
        }

        character::pasteInto(
            getSheet(),
            *mark.selectedFrame / character::kCharacterFrames,
            *mark.selectedFrame % character::kCharacterFrames,
            character::getSelectionOrigin(*mark.selection),
            *mark.floatingPatchBuffer);

        mark.floatingPatchBuffer.reset();
        touch();
    }

    void CharacterSheetView::mirrorSelection(IEditSteps &editSteps)
    {
        if (!mark.selection.has_value()
            || !mark.selectedFrame.has_value())
        {
            return;
        }

        const auto way = *mark.selectedFrame
                         / character::kCharacterFrames;
        const auto frame = *mark.selectedFrame
                           % character::kCharacterFrames;

        if (mark.floatingPatchBuffer.has_value())
        {
            mark.floatingPatchBuffer =
                character::getMirroredHorizontally(
                    *mark.floatingPatchBuffer);

            return;
        }

        editSteps.pushUndo();
        character::pasteInto(
            getSheet(),
            way,
            frame,
            character::getSelectionOrigin(*mark.selection),
            character::getMirroredHorizontally(
                character::copiedFrom(
                    getSheet(),
                    way,
                    frame,
                    *mark.selection)));
        touch();
    }


    bool CharacterSheetView::consumePress(
        const ViewContext &viewContext,
        const input::PointerButtonPressed &downPressed)
    {
        if (downPressed.button != input::MouseButton::Left
            && downPressed.button != input::MouseButton::Right)
        {
            return false;
        }

        const auto projectToScreen =
            viewContext.render.viewportRenderer.getViewport().toCanvas(
                gfx::Point{
                    .x = downPressed.position.x,
                    .y = downPressed.position.y});

        viewContext.workbench.pointer.pointerOnCanvas = gfx::PointF{
            static_cast<float>(projectToScreen.x),
            static_cast<float>(projectToScreen.y)};

        const auto chosenCell =
            characterAt(camera::kCanvasSize, viewContext.workbench.pointer.pointerOnCanvas);

        if (chosenCell.has_value())
        {
            commitFloatingPatch();
            mark.selection.reset();

            if (downPressed.button
                == input::MouseButton::Right)
            {
                mark.selectedFrame.reset();
            }
            else
            {
                mark.selectedFrame = *chosenCell;
            }

            return true;
        }

        if (!mark.selectedFrame.has_value())
        {
            return true;
        }

        const auto pixel = character::characterPixelAt(
            getCharacterCanvasRect(camera::kCanvasSize),
            viewContext.workbench.pointer.pointerOnCanvas);

        if (!pixel.has_value())
        {
            return true;
        }

        if (downPressed.button == input::MouseButton::Right
            && mark.selection.has_value())
        {
            commitFloatingPatch();
            mark.selection.reset();

            return true;
        }

        if (downPressed.button == input::MouseButton::Left
            && (viewContext.heldModifiers.shift
                || viewContext.workbench.preferences.paint == map::Paint::Select))
        {
            if (mark.selection.has_value()
                && character::isSelectionContains(
                    *mark.selection, *pixel))
            {
                mark.draggingPatch = true;
                mark.grabbedMarkSelection =
                    mark.selection;
                mark.grabbedAtCell = pixel;

                if (!mark.floatingPatchBuffer.has_value())
                {
                    mark.floatingPatchBuffer =
                        character::cutFrom(
                            getSheet(),
                            *mark.selectedFrame
                                / character::kCharacterFrames,
                            *mark.selectedFrame
                                % character::kCharacterFrames,
                            *mark.selection);
                    touch();
                }

                return true;
            }

            commitFloatingPatch();
            mark.selecting = true;
            mark.selection = character::PixelSelection{
                .fromCell = *pixel, .toCell = *pixel};

            return true;
        }

        commitFloatingPatch();
        mark.selection.reset();
        viewContext.workbench.stroke.erases =
            downPressed.button == input::MouseButton::Right;
        viewContext.editSteps.pushUndo();

        const auto color = character::getCharacterPaletteColor(
            viewContext.document.map.paletteColors,
            viewContext.workbench.stroke.erases
                ? character::kTransparentInk
                : viewContext.workbench.inkPicker.activeInk);

        if (viewContext.workbench.preferences.paint == map::Paint::Fill
            && !viewContext.workbench.stroke.erases)
        {
            character::paintCharacterFill(
                getSheet(),
                *mark.selectedFrame
                    / character::kCharacterFrames,
                *mark.selectedFrame
                    % character::kCharacterFrames,
                *pixel,
                color);
        }
        else
        {
            character::paintCharacter(
                getSheet(),
                *mark.selectedFrame
                    / character::kCharacterFrames,
                *mark.selectedFrame
                    % character::kCharacterFrames,
                *pixel,
                color);
            viewContext.workbench.stroke.brushAtCell = pixel;
            viewContext.workbench.stroke.active = true;
        }

        touch();

        return true;

        return true;
    }


    void CharacterSheetView::trackPointer(const ViewContext &viewContext)
    {
        if (true)
        {
            const auto characterCell =
                characterAt(camera::kCanvasSize, viewContext.workbench.pointer.pointerOnCanvas);

            mark.hoveredWayRow =
                characterCell.has_value()
                    ? std::optional<std::size_t>{
                          *characterCell / character::kCharacterFrames}
                    : std::nullopt;
        }


        if (mark.selecting || mark.draggingPatch)
        {
            const auto pixel = character::characterPixelAt(
                getCharacterCanvasRect(camera::kCanvasSize),
                viewContext.workbench.pointer.pointerOnCanvas);

            if (pixel.has_value() && mark.selecting
                && mark.selection.has_value())
            {
                mark.selection->toCell = *pixel;
            }

            if (pixel.has_value() && mark.draggingPatch
                && mark.grabbedMarkSelection.has_value()
                && mark.grabbedAtCell.has_value())
            {
                mark.selection = character::getMovedSelection(
                    *mark.grabbedMarkSelection,
                    static_cast<std::int32_t>(
                        pixel->column)
                        - static_cast<std::int32_t>(
                            mark.grabbedAtCell->column),
                    static_cast<std::int32_t>(
                        pixel->row)
                        - static_cast<std::int32_t>(
                            mark.grabbedAtCell->row));
            }
        }


        if (viewContext.workbench.stroke.active
            && mark.selectedFrame.has_value())
        {
            const auto pixel = character::characterPixelAt(
                getCharacterCanvasRect(camera::kCanvasSize),
                viewContext.workbench.pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                character::paintCharacterLine(
                    getSheet(),
                    *mark.selectedFrame
                        / character::kCharacterFrames,
                    *mark.selectedFrame
                        % character::kCharacterFrames,
                    viewContext.workbench.stroke.brushAtCell.value_or(*pixel),
                    *pixel,
                    character::getCharacterPaletteColor(
                        viewContext.document.map.paletteColors,
                        viewContext.workbench.stroke.erases ? character::kTransparentInk
                                     : viewContext.workbench.inkPicker.activeInk));
                viewContext.workbench.stroke.brushAtCell = pixel;
                touch();
            }
        }
    }


    namespace
    {
        [[nodiscard]] bool isChordMatched(
            const ViewContext &viewContext,
            const input::KeyPressed &pressedKey,
            const Action action)
        {
            return viewContext.workbench.keyBench.matchesChord(
                action, pressedKey.key, viewContext.heldModifiers);
        }
    }

    bool CharacterSheetView::consumeKey(
        const ViewContext &viewContext, const input::KeyPressed &pressedKey)
    {
        if (isChordMatched(viewContext, pressedKey, Action::Mirror)
            && !pressedKey.repeat)
        {
            mirrorSelection(viewContext.editSteps);

            return true;
        }

        if (pressedKey.repeat || !mark.selection.has_value()
            || !mark.selectedFrame.has_value())
        {
            return false;
        }

        const auto way = *mark.selectedFrame
                         / character::kCharacterFrames;
        const auto frame =
            *mark.selectedFrame % character::kCharacterFrames;

        if (isChordMatched(viewContext, pressedKey, Action::Copy))
        {
            mark.clipboardBuffer =
                mark.floatingPatchBuffer.has_value()
                      ? *mark.floatingPatchBuffer
                      : character::copiedFrom(
                            getSheet(),
                            way,
                            frame,
                            *mark.selection);
        }

        if (isChordMatched(viewContext, pressedKey, Action::Cut))
        {
            mark.clipboardBuffer =
                mark.floatingPatchBuffer.has_value()
                      ? *mark.floatingPatchBuffer
                      : character::cutFrom(
                            getSheet(),
                            way,
                            frame,
                            *mark.selection);
            mark.floatingPatchBuffer.reset();
            touch();
        }

        if (isChordMatched(viewContext, pressedKey, Action::Paste)
            && !mark.clipboardBuffer.pixelColors.empty())
        {
            commitFloatingPatch();
            viewContext.editSteps.pushUndo();
            character::pasteInto(
                getSheet(),
                way,
                frame,
                character::getSelectionOrigin(*mark.selection),
                mark.clipboardBuffer);
            touch();
        }

        if (isChordMatched(viewContext, pressedKey, Action::Delete))
        {
            if (mark.floatingPatchBuffer.has_value())
            {
                mark.floatingPatchBuffer.reset();
            }
            else
            {
                viewContext.editSteps.pushUndo();
                (void)character::cutFrom(
                    getSheet(),
                    way,
                    frame,
                    *mark.selection);
            }

            mark.selection.reset();
            touch();
        }

        return false;
    }


    bool CharacterSheetView::takesPaintKeys() const noexcept
    {
        return true;
    }

    bool CharacterSheetView::offersPaint(
        const map::Paint paint) const noexcept
    {
        return paint == map::Paint::Select || IEditorView::offersPaint(paint);
    }

}
