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
                render::checkered(character::kCharacterCellSize, 4));
        }
    }

    void CharacterSheetView::takeSkins(
        gfx::ViewportRenderer &viewportRenderer,
        std::vector<gfx::Bitmap> skinBitmaps)
    {
        rosterSkins.take(viewportRenderer, std::move(skinBitmaps));
        editingAt = std::min(editingAt, rosterSkins.size() - 1);
        editedSheet = rosterSkins.sheets().at(editingAt);
        sheetDirty = true;
    }

    const std::vector<gfx::Bitmap> &CharacterSheetView::skins()
        const noexcept
    {
        return rosterSkins.sheets();
    }

    std::vector<gfx::Bitmap> CharacterSheetView::skinsAsDrawn() const
    {
        auto sheets = rosterSkins.sheets();

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

    std::size_t CharacterSheetView::editing() const noexcept
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
        if (editingAt == skinIndex || skinIndex >= rosterSkins.size())
        {
            return;
        }

        rosterSkins.lay(viewportRenderer, editingAt, editedSheet);
        editingAt = skinIndex;
        editedSheet = rosterSkins.sheets().at(skinIndex);
        sheetDirty = true;
    }

    void CharacterSheetView::repaint(
        gfx::ViewportRenderer &viewportRenderer,
        const std::size_t skinIndex,
        gfx::Bitmap skinBitmap)
    {
        rosterSkins.lay(viewportRenderer, skinIndex, std::move(skinBitmap));
    }

    gfx::Bitmap &CharacterSheetView::sheet() noexcept
    {
        return editedSheet;
    }

    const gfx::Bitmap &CharacterSheetView::sheet() const noexcept
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

    gfx::ITexture *CharacterSheetView::texture() const noexcept
    {
        return sheetTexture.get();
    }

    gfx::ITexture *CharacterSheetView::skinTexture(
        const std::size_t skinIndex) const noexcept
    {
        return rosterSkins.picture(skinIndex);
    }

    gfx::ITexture *CharacterSheetView::checker() const noexcept
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
                    characterPlace(camera::kCanvasSize, way, frame);
                const auto right =
                    place.originPoint.x + place.size.width;
                const auto foot =
                    place.originPoint.y + place.size.height;

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
                    character::characterSource(way, frame),
                    place,
                    chosenFrame ? kWhiteColor : kDisabledTintColor);

                for (const auto &[fromPoint, toPoint] :
                     {std::pair{
                          place.originPoint,
                          antwika::gfx::PointF{
                              right, place.originPoint.y}},
                      std::pair{
                          antwika::gfx::PointF{
                              place.originPoint.x, foot},
                          antwika::gfx::PointF{
                              right, foot}},
                      std::pair{
                          place.originPoint,
                          antwika::gfx::PointF{
                              place.originPoint.x, foot}},
                      std::pair{
                          antwika::gfx::PointF{
                              right, place.originPoint.y},
                          antwika::gfx::PointF{
                              right, foot}}})
                {
                    viewportRenderer.drawLine(
                        fromPoint,
                        toPoint,
                        chosenFrame ? kSelectionAccentColor : kGridLineColor);
                }
            }
        }

        const auto drawnAt =
            characterCanvasRect(camera::kCanvasSize);

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
                        character::characterPixelPlace(drawnAt, pixelCell),
                        character::characterPixelColor(
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
                    character::selectionOrigin(*mark.selection);

                for (std::uint32_t row = 0;
                     row < mark.floatingPatchBuffer->size.height;
                     ++row)
                {
                    for (std::uint32_t column = 0;
                         column < mark.floatingPatchBuffer->size.width;
                         ++column)
                    {
                        viewportRenderer.drawRect(
                            character::characterPixelPlace(
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
                    selectionRect(drawnAt, *mark.selection);
                const auto right =
                    place.originPoint.x + place.size.width;
                const auto foot =
                    place.originPoint.y + place.size.height;

                for (const auto &[fromPoint, toPoint] :
                     {std::pair{
                          place.originPoint,
                          antwika::gfx::PointF{
                              right, place.originPoint.y}},
                      std::pair{
                          antwika::gfx::PointF{
                              place.originPoint.x, foot},
                          antwika::gfx::PointF{
                              right, foot}},
                      std::pair{
                          place.originPoint,
                          antwika::gfx::PointF{
                              place.originPoint.x, foot}},
                      std::pair{
                          antwika::gfx::PointF{
                              right, place.originPoint.y},
                          antwika::gfx::PointF{
                              right, foot}}})
                {
                    viewportRenderer.drawLine(fromPoint, toPoint,
                    kSelectionAccentColor);
                }
            }

            viewportRenderer.drawText(
                antwika::gfx::PointF{
                    drawnAt.originPoint.x,
                    drawnAt.originPoint.y + drawnAt.size.height
                        + 4.0F},
                std::string(
                    character::directionName(
                        *mark.selectedFrame
                            / character::kCharacterFrames)),
                1,
                kTextColor);
        }
    }

}
