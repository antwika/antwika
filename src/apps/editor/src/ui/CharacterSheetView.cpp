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
        std::vector<gfx::Bitmap> skinBitmaps)
    {
        rosterSkins.take(viewportRenderer, std::move(skinBitmaps));
        editingAt = std::min(editingAt, rosterSkins.getSize() - 1);
        editedSheet = rosterSkins.getSheets().at(editingAt);
        sheetDirty = true;
    }

    const std::vector<gfx::Bitmap> &CharacterSheetView::getSkins()
        const noexcept
    {
        return rosterSkins.getSheets();
    }

    std::vector<gfx::Bitmap> CharacterSheetView::getSkinsAsDrawn() const
    {
        auto sheets = rosterSkins.getSheets();

        if (editingAt < sheets.size())
        {
            sheets.at(editingAt) = editedSheet;
        }

        return sheets;
    }

    void CharacterSheetView::keepEdits(gfx::ViewportRenderer &viewportRenderer)
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
        gfx::ViewportRenderer &viewportRenderer, const std::size_t skinIndex)
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

    gfx::ITexture *CharacterSheetView::getSkinTexture(
        const std::size_t skinIndex) const noexcept
    {
        return rosterSkins.getPicture(skinIndex);
    }

    gfx::ITexture *CharacterSheetView::getChecker() const noexcept
    {
        return sheetCheckerTexture.get();
    }

    void CharacterSheetView::draw(gfx::ViewportRenderer &viewportRenderer) const
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

}
