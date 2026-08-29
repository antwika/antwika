#include "antwika/editor/ui/GizmoSheet.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <antwika/assets/MapAssets.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/map/MapFile.hpp>

#include "antwika/editor/ui/IconSheet.hpp"

namespace antwika::editor
{

    namespace
    {

        [[nodiscard]] gfx::Size getGizmoSheetSize()
        {
            return gfx::Size{
                .width = kIconCellSize.width
                         * static_cast<std::uint32_t>(
                             enums::kCount<GizmoKind>),
                .height = kIconCellSize.height};
        }

    }

    gfx::Bitmap getBlankGizmoSheet()
    {
        const auto size = getGizmoSheetSize();

        return gfx::Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width) * size.height
                * gfx::kBytesPerPixel)};
    } // GCOVR_EXCL_LINE

    gfx::Bitmap getLoadGizmoSheet(
        const std::string &mapPath, const std::string_view app)
    {
        if (!std::filesystem::exists(
                map::getSharedTexturePath(mapPath, kGizmoSheet)))
        {
            return getBlankGizmoSheet();
        }

        auto sheet =
            assets::getReadSharedOrBundled(mapPath, kGizmoSheet, app);
        const auto fullSize = getGizmoSheetSize();

        if (sheet.size.height != fullSize.height
            || sheet.size.width % kIconCellSize.width != 0
            || sheet.size.width == 0
            || sheet.size.width > fullSize.width)
        {
            throw gfx::GfxError(
                "the gizmo sheet is not a row of gizmo cells");
        }

        if (sheet.size == fullSize)
        {
            return sheet;
        }

        auto grownSheet = getBlankGizmoSheet();
        const auto rowBytes =
            static_cast<std::size_t>(sheet.size.width)
            * gfx::kBytesPerPixel;
        const auto grownRowBytes =
            static_cast<std::size_t>(fullSize.width) * gfx::kBytesPerPixel;

        for (std::uint32_t row = 0; row < sheet.size.height; ++row)
        {
            std::copy_n(
                sheet.pixels.begin()
                    + static_cast<std::ptrdiff_t>(row * rowBytes),
                rowBytes,
                grownSheet.pixels.begin()
                    + static_cast<std::ptrdiff_t>(row * grownRowBytes));
        }

        return grownSheet;
    } // GCOVR_EXCL_LINE

    bool isGizmoDrawn(
        const gfx::Bitmap &sheetBitmap, const std::size_t gizmoIndex)
    {
        for (std::uint32_t row = 0; row < kIconCellSize.height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < kIconCellSize.width;
                 ++column)
            {
                if (getIconPixelColor(
                        sheetBitmap,
                        gizmoIndex,
                        geometry::GridCell{column, row})
                        .alpha
                    != 0)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void wipeGizmo(gfx::Bitmap &sheetBitmap, const std::size_t gizmoIndex)
    {
        for (std::uint32_t row = 0; row < kIconCellSize.height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < kIconCellSize.width;
                 ++column)
            {
                setIconPixel(
                    sheetBitmap,
                    gizmoIndex,
                    geometry::GridCell{column, row},
                    gfx::Color{.alpha = 0});
            }
        }
    }

}
