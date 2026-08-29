#include "antwika/editor/ui/IconsView.hpp"

#include <chrono>
#include <cstdint>
#include <utility>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/input/MouseButton.hpp>

#include <antwika/render/Checkerboard.hpp>

#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/IconSheet.hpp"

namespace antwika::editor
{

    namespace
    {
        [[nodiscard]] gfx::RectF getSheetRect(
            const ViewContext &viewContext)
        {
            return viewContext.workbench.sheetView.sheetRect.value_or(
                getIconSheetBounds(camera::kCanvasSize));
        }

        [[nodiscard]] gfx::RectF getDrawRect(
            const ViewContext &viewContext)
        {
            return viewContext.workbench.sheetView.canvasRect.value_or(
                getIconDrawBounds(camera::kCanvasSize));
        }
    }

    void IconsView::open(
        gfx::ViewportRenderer &viewportRenderer,
        gfx::Bitmap sheetBitmap)
    {
        iconSheet = std::move(sheetBitmap);
        iconsTexture = viewportRenderer.createTexture(iconSheet);
        iconsUnsaved = false;

        if (!iconCheckerTexture)
        {
            iconCheckerTexture = viewportRenderer.createTexture(
                render::getCheckerboardBitmap(kIconCellSize, 4));
        }
    }

    gfx::ITexture *IconsView::getTexture() const noexcept
    {
        return iconsTexture.get();
    }

    const gfx::Bitmap &IconsView::getSheet() const noexcept
    {
        return iconSheet;
    }

    gfx::ITexture *IconsView::getChecker() const noexcept
    {
        return iconCheckerTexture.get();
    }

    std::optional<std::size_t> IconsView::getPickedIndex() const noexcept
    {
        return iconPicked;
    }

    bool IconsView::isUnsaved() const noexcept
    {
        return iconsUnsaved;
    }

    void IconsView::keep() noexcept
    {
        iconsUnsaved = false;
    }

    void IconsView::pick(const std::optional<std::size_t> iconIndex) noexcept
    {
        iconPicked = iconIndex;
    }

    void IconsView::paint(
        gfx::ViewportRenderer &viewportRenderer,
        const geometry::GridCell pixelCell,
        const bool erases)
    {
        setIconPixel(
            iconSheet,
            *iconPicked,
            pixelCell,
            erases ? gfx::Color{.alpha = 0} : kWhiteColor);
        iconsTexture = viewportRenderer.createTexture(iconSheet);
        iconsUnsaved = true;
    }

    void IconsView::drawSheet(
        gfx::ViewportRenderer &viewportRenderer,
        const gfx::RectF sheetRect,
        const gfx::RectF drawRect) const
    {
        const auto count =
            antwika::editor::getIconCount(iconSheet.size);

        viewportRenderer.drawRect(sheetRect, kPanelColor);
        viewportRenderer.drawRect(drawRect, kPanelColor);

        {
            const auto sheetScope = viewportRenderer.clipScope(sheetRect);

            for (std::size_t index = 0; index < count; ++index)
            {
                const auto iconChosen = iconPicked == index;
                const auto place = antwika::editor::getIconCellRect(
                    sheetRect, count, index);

                viewportRenderer.drawTexture(
                    *iconCheckerTexture,
                    antwika::gfx::RectF(
                        {0.0F, 0.0F},
                        {static_cast<float>(
                             antwika::editor::kIconCellSize.width),
                         static_cast<float>(
                             antwika::editor::kIconCellSize.height)}),
                    place,
                    kWhiteColor);
                viewportRenderer.drawTexture(
                    *iconsTexture,
                    antwika::editor::getIconSource(index),
                    place,
                    iconChosen ? kWhiteColor : kDisabledTintColor);

                drawOutline(
                    viewportRenderer,
                    place,
                    iconChosen ? kSelectionAccentColor : kGridLineColor);
            }
        }

        if (iconPicked.has_value() && *iconPicked < count)
        {
            const auto drawScope = viewportRenderer.clipScope(drawRect);
            const auto drawnAt =
                antwika::editor::getEditedIconRect(drawRect);

            viewportRenderer.drawTexture(
                *iconCheckerTexture,
                antwika::gfx::RectF(
                    {0.0F, 0.0F},
                    {static_cast<float>(
                         antwika::editor::kIconCellSize.width),
                     static_cast<float>(
                         antwika::editor::kIconCellSize.height)}),
                drawnAt,
                kWhiteColor);

            for (std::uint32_t row = 0;
                 row < antwika::editor::kIconCellSize.height;
                 ++row)
            {
                for (std::uint32_t column = 0;
                     column < antwika::editor::kIconCellSize.width;
                     ++column)
                {
                    const antwika::geometry::GridCell pixelCell{
                        column, row};

                    viewportRenderer.drawRect(
                        antwika::editor::getIconPixelRect(
                            drawnAt, pixelCell),
                        antwika::editor::getIconPixelColor(
                            iconSheet, *iconPicked, pixelCell));
                }
            }
        }
    }

    std::size_t IconsView::getCount() const
    {
        return getIconCount(iconSheet.size);
    }


    bool IconsView::claims(
        const View shownView, const bool playing) const noexcept
    {
        return !playing && shownView == View::Icons;
    }

    std::string IconsView::getStatusText(const ViewContext &) const
    {
        return "4 icons - click an icon to take it up - "
               "lmb paints - rmb rubs out - saved with "
               "the map";
    }

    void IconsView::draw(
        const ViewContext &viewContext, const ui::Frame &)
    {
        drawSheet(
            viewContext.render.viewportRenderer,
            getSheetRect(viewContext),
            getDrawRect(viewContext));
    }

    bool IconsView::consumePress(
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

        const auto count = getCount();
        const auto chosenCell = iconCellAt(
            getSheetRect(viewContext),
            count,
            viewContext.workbench.pointer.pointerOnCanvas);

        if (chosenCell.has_value())
        {
            pick(
                downPressed.button == input::MouseButton::Right
                    ? std::nullopt
                    : std::optional{*chosenCell});

            return true;
        }

        if (!iconPicked.has_value() || *iconPicked >= count)
        {
            return true;
        }

        const auto pixel = iconPixelAt(
            getEditedIconRect(getDrawRect(viewContext)),
            viewContext.workbench.pointer.pointerOnCanvas);

        if (!pixel.has_value())
        {
            return true;
        }

        viewContext.workbench.stroke.erases =
            downPressed.button == input::MouseButton::Right;
        paint(viewContext.render.viewportRenderer, *pixel, viewContext.workbench.stroke.erases);
        viewContext.document.markDirty();
        viewContext.workbench.stroke.brushAtCell = pixel;
        viewContext.workbench.stroke.active = true;

        return true;
    }

    void IconsView::trackPointer(const ViewContext &viewContext)
    {
        if (!viewContext.workbench.stroke.active || !iconPicked.has_value())
        {
            return;
        }

        const auto pixel = iconPixelAt(
            getEditedIconRect(getDrawRect(viewContext)),
            viewContext.workbench.pointer.pointerOnCanvas);

        if (!pixel.has_value())
        {
            return;
        }

        paint(viewContext.render.viewportRenderer, *pixel, viewContext.workbench.stroke.erases);
        viewContext.workbench.stroke.brushAtCell = pixel;
    }

}
