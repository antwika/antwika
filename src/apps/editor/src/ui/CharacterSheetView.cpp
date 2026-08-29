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

#include "antwika/editor/editor/state/PanelSizes.hpp"
#include "antwika/editor/ui/CharacterView.hpp"
#include "antwika/editor/ui/EditorLook.hpp"

namespace antwika::editor
{

    namespace
    {
        struct FrameCoords final
        {
            std::size_t way;

            std::size_t frame;
        };

        [[nodiscard]] FrameCoords frameCoordsOf(
            const std::size_t selectedFrame) noexcept
        {
            return FrameCoords{
                .way = selectedFrame / character::kCharacterFrames,
                .frame = selectedFrame % character::kCharacterFrames};
        }

        [[nodiscard]] float railWidthOf(const ViewContext &viewContext)
        {
            return getRailWidthOnCanvas(
                viewContext.workbench.preferences.panelSizes,
                viewContext.render.viewportRenderer.getWindowSize(),
                camera::kCanvasSize);
        }

        [[nodiscard]] gfx::RectF getSheetRect(
            const ViewContext &viewContext)
        {
            return viewContext.workbench.sheetView.sheetRect.value_or(
                getCharacterSheetBounds(camera::kCanvasSize));
        }

        [[nodiscard]] gfx::RectF getDrawRect(
            const ViewContext &viewContext)
        {
            return viewContext.workbench.sheetView.canvasRect.value_or(
                getCharacterDrawBounds(
                    camera::kCanvasSize, railWidthOf(viewContext)));
        }
    }

    void CharacterSheetView::open(
        gfx::ViewportRenderer &viewportRenderer, gfx::Bitmap sheetBitmap)
    {
        editedBitmap = std::move(sheetBitmap);
        sheetTexture = viewportRenderer.createTexture(editedBitmap);
        sheetDirty = false;

        if (!sheetCheckerTexture)
        {
            sheetCheckerTexture = viewportRenderer.createTexture(
                render::getCheckerboardBitmap(character::kCharacterCellSize, 4));
        }
    }

    void CharacterSheetView::takeSkins(
        gfx::ViewportRenderer &viewportRenderer,
        render::CharacterSkins &characterSkins,
        std::vector<gfx::Bitmap> skinBitmaps)
    {
        characterSkins.take(viewportRenderer, std::move(skinBitmaps));
        editingAt = std::min(editingAt, characterSkins.getSize() - 1);
        editedBitmap = characterSkins.getSheets().at(editingAt);
        sheetDirty = true;
    }

    
    std::vector<gfx::Bitmap> CharacterSheetView::getSkinsAsDrawn(
        const render::CharacterSkins &characterSkins) const
    {
        auto sheets = characterSkins.getSheets();

        if (editingAt < sheets.size())
        {
            sheets.at(editingAt) = editedBitmap;
        }

        return sheets;
    }

    void CharacterSheetView::keepEdits(
        gfx::ViewportRenderer &viewportRenderer,
        render::CharacterSkins &characterSkins)
    {
        characterSkins.lay(viewportRenderer, editingAt, editedBitmap);
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
        render::CharacterSkins &characterSkins,
        const std::size_t skinIndex)
    {
        if (editingAt == skinIndex || skinIndex >= characterSkins.getSize())
        {
            return;
        }

        characterSkins.lay(viewportRenderer, editingAt, editedBitmap);
        editingAt = skinIndex;
        editedBitmap = characterSkins.getSheets().at(skinIndex);
        sheetDirty = true;
    }

    void CharacterSheetView::repaint(
        gfx::ViewportRenderer &viewportRenderer,
        render::CharacterSkins &characterSkins,
        const std::size_t skinIndex,
        gfx::Bitmap skinBitmap)
    {
        characterSkins.lay(viewportRenderer, skinIndex, std::move(skinBitmap));
    }

    gfx::Bitmap &CharacterSheetView::getSheet() noexcept
    {
        return editedBitmap;
    }

    const gfx::Bitmap &CharacterSheetView::getSheet() const noexcept
    {
        return editedBitmap;
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

        sheetTexture = viewportRenderer.createTexture(editedBitmap);
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
        gfx::ViewportRenderer &viewportRenderer,
        const gfx::RectF sheetRect,
        const gfx::RectF drawRect) const
    {
        viewportRenderer.drawRect(sheetRect, kPanelColor);
        viewportRenderer.drawRect(drawRect, kPanelColor);

        {
            const auto sheetScope = viewportRenderer.clipScope(sheetRect);

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
                        getCharacterPlace(sheetRect, way, frame);

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
                        chosenFrame ? kSelectionAccentColor
                                    : kGridLineColor);
                }
            }
        }

        const auto drawnAt =
            getCharacterCanvasRect(drawRect);

        if (mark.selectedFrame.has_value())
        {
            const auto drawScope = viewportRenderer.clipScope(drawRect);

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
                    const auto coords =
                        frameCoordsOf(*mark.selectedFrame);

                    viewportRenderer.drawRect(
                        character::getCharacterPixelPlace(drawnAt, pixelCell),
                        character::getCharacterPixelColor(
                            editedBitmap,
                            coords.way,
                            coords.frame,
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
                        frameCoordsOf(*mark.selectedFrame).way)),
                antwika::gfx::TextScale{.multiplier = 1},
                kTextColor);
        }
    }


    bool CharacterSheetView::claims(
        const View shownView, const bool playing) const noexcept
    {
        return !playing && shownView == View::Character;
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
        drawSheet(
            viewContext.render.viewportRenderer,
            getSheetRect(viewContext),
            getDrawRect(viewContext));
    }


    void CharacterSheetView::commitFloatingPatch()
    {
        if (!mark.floatingPatchBuffer.has_value()
            || !mark.selection.has_value()
            || !mark.selectedFrame.has_value())
        {
            return;
        }

        const auto coords = frameCoordsOf(*mark.selectedFrame);

        character::pasteInto(
            getSheet(),
            coords.way,
            coords.frame,
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

        const auto coords = frameCoordsOf(*mark.selectedFrame);

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
            coords.way,
            coords.frame,
            character::getSelectionOrigin(*mark.selection),
            character::getMirroredHorizontally(
                character::copiedFrom(
                    getSheet(),
                    coords.way,
                    coords.frame,
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
            characterAt(
                getSheetRect(viewContext),
                viewContext.workbench.pointer.pointerOnCanvas);

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
            getCharacterCanvasRect(getDrawRect(viewContext)),
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
                || viewContext.workbench.preferences.paint == Paint::Select))
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
                    const auto coords =
                        frameCoordsOf(*mark.selectedFrame);

                    mark.floatingPatchBuffer =
                        character::cutFrom(
                            getSheet(),
                            coords.way,
                            coords.frame,
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

        if (!viewContext.workbench.stroke.erases
            && shapePixelsOf(viewContext.workbench.preferences.paint)
                   != nullptr)
        {
            viewContext.workbench.stroke.lineFromCell = pixel;

            return true;
        }

        viewContext.editSteps.pushUndo();

        const auto color = character::getCharacterPaletteColor(
            viewContext.document.map.paletteColors,
            viewContext.workbench.stroke.erases
                ? character::kTransparentInk
                : viewContext.workbench.inkPicker.activeInk);

        beginStroke(
            viewContext.workbench.stroke.erases
                ? Paint::Brush
                : viewContext.workbench.preferences.paint,
            *pixel,
            viewContext.workbench.stroke,
            createPaintSurface(color));

        return true;
    }


    void CharacterSheetView::trackPointer(const ViewContext &viewContext)
    {
        const auto characterCell =
            characterAt(
                getSheetRect(viewContext),
                viewContext.workbench.pointer.pointerOnCanvas);

        mark.hoveredWayRow =
            characterCell.has_value()
                ? std::optional<std::size_t>{
                      *characterCell / character::kCharacterFrames}
                : std::nullopt;


        if (mark.selecting || mark.draggingPatch)
        {
            const auto pixel = character::characterPixelAt(
                getCharacterCanvasRect(getDrawRect(viewContext)),
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
                getCharacterCanvasRect(getDrawRect(viewContext)),
                viewContext.workbench.pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                dragStroke(
                    *pixel,
                    viewContext.workbench.stroke,
                    createPaintSurface(character::getCharacterPaletteColor(
                        viewContext.document.map.paletteColors,
                        viewContext.workbench.stroke.erases
                            ? character::kTransparentInk
                            : viewContext.workbench.inkPicker.activeInk)));
            }
        }
    }

    bool CharacterSheetView::consumeRelease(
        const ViewContext &viewContext,
        const input::PointerButtonReleased &upReleased)
    {
        if (upReleased.button != input::MouseButton::Left)
        {
            return false;
        }

        finishShapedStroke(viewContext, upReleased);

        mark.selecting = false;
        mark.draggingPatch = false;
        mark.grabbedMarkSelection.reset();
        mark.grabbedAtCell.reset();

        return true;
    }

    void CharacterSheetView::finishShapedStroke(
        const ViewContext &viewContext,
        const input::PointerButtonReleased &upReleased)
    {
        auto &stroke = viewContext.workbench.stroke;

        if (!mark.selectedFrame.has_value())
        {
            stroke.lineFromCell.reset();

            return;
        }

        const auto projectToScreen =
            viewContext.render.viewportRenderer.getViewport().toCanvas(
                gfx::Point{
                    .x = upReleased.position.x,
                    .y = upReleased.position.y});
        const auto pixel = character::characterPixelAt(
            getCharacterCanvasRect(getDrawRect(viewContext)),
            gfx::PointF{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)});

        if (pixel.has_value())
        {
            endShapedStroke(
                viewContext.workbench.preferences.paint,
                *pixel,
                stroke,
                createPaintSurface(
                    character::getCharacterPaletteColor(
                        viewContext.document.map.paletteColors,
                        viewContext.workbench.inkPicker.activeInk)),
                viewContext.editSteps);
        }

        stroke.lineFromCell.reset();
    }

    PaintSurface CharacterSheetView::createPaintSurface(const gfx::Color color)
    {
        const auto coords = frameCoordsOf(*mark.selectedFrame);

        return PaintSurface{
            .paintCells =
                [this, coords, color](
                    const std::span<const geometry::GridCell> cells)
            {
                for (const auto cell : cells)
                {
                    character::paintCharacter(
                        getSheet(), coords.way, coords.frame, cell, color);
                }
            },
            .paintFill =
                [this, coords, color](const geometry::GridCell cell)
            {
                character::paintCharacterFill(
                    getSheet(), coords.way, coords.frame, cell, color);
            },
            .touch = [this] { touch(); }};
    }

    SheetMark &CharacterSheetView::getMark() noexcept
    {
        return mark;
    }

    const SheetMark &CharacterSheetView::getMark() const noexcept
    {
        return mark;
    }

    void CharacterSheetView::dropSelection() noexcept
    {
        mark.selection.reset();
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

        const auto coords = frameCoordsOf(*mark.selectedFrame);

        if (isChordMatched(viewContext, pressedKey, Action::Copy))
        {
            mark.clipboardBuffer =
                mark.floatingPatchBuffer.has_value()
                      ? *mark.floatingPatchBuffer
                      : character::copiedFrom(
                            getSheet(),
                            coords.way,
                            coords.frame,
                            *mark.selection);

            return true;
        }

        if (isChordMatched(viewContext, pressedKey, Action::Cut))
        {
            mark.clipboardBuffer =
                mark.floatingPatchBuffer.has_value()
                      ? *mark.floatingPatchBuffer
                      : character::cutFrom(
                            getSheet(),
                            coords.way,
                            coords.frame,
                            *mark.selection);
            mark.floatingPatchBuffer.reset();
            touch();

            return true;
        }

        if (isChordMatched(viewContext, pressedKey, Action::Paste)
            && !mark.clipboardBuffer.pixelColors.empty())
        {
            commitFloatingPatch();
            viewContext.editSteps.pushUndo();
            character::pasteInto(
                getSheet(),
                coords.way,
                coords.frame,
                character::getSelectionOrigin(*mark.selection),
                mark.clipboardBuffer);
            touch();

            return true;
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
                    coords.way,
                    coords.frame,
                    *mark.selection);
            }

            mark.selection.reset();
            touch();

            return true;
        }

        return false;
    }


    bool CharacterSheetView::takesPaintKeys() const noexcept
    {
        return true;
    }

    bool CharacterSheetView::offersPaint(
        const Paint paint) const noexcept
    {
        return paint == Paint::Select || IEditorView::offersPaint(paint);
    }

}
