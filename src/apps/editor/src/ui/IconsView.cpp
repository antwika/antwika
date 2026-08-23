#include "antwika/editor/ui/IconsView.hpp"

#include <chrono>
#include <cstdint>
#include <utility>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

#include <antwika/render/Checkerboard.hpp>
#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/IconSheet.hpp"

namespace antwika::editor
{

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
                render::getCheckered(kIconCellSize, 4));
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

    std::optional<std::size_t> IconsView::getPicked() const noexcept
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

    void IconsView::draw(gfx::ViewportRenderer &viewportRenderer) const
    {
        const auto count =
            antwika::editor::getIconCount(iconSheet.size);

        for (std::size_t index = 0; index < count; ++index)
        {
            const auto iconChosen = iconPicked == index;
            const auto place = antwika::editor::getIconCellRect(
                camera::kCanvasSize, count, index);

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

        if (iconPicked.has_value() && *iconPicked < count)
        {
            const auto drawnAt =
                antwika::editor::getEditedIconRect(camera::kCanvasSize);

            viewportRenderer.drawRect(drawnAt, kPanelColor);
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

}
