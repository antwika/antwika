#include "antwika/map_editor/PlaceholderSheets.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/autotile/SheetLayout.hpp>

namespace antwika::map_editor
{

    namespace
    {
        using antwika::autotile::kSheetHeight;
        using antwika::autotile::kSheetWidth;
        using antwika::gfx::Bitmap;
        using antwika::gfx::Color;
        using antwika::tilemap::TerrainClass;

        [[nodiscard]] bool patternInk(
            const TerrainClass terrain,
            const std::int32_t x,
            const std::int32_t y)
        {
            switch (terrain)
            {
                case TerrainClass::Floor:
                    return (x == 3 && y == 5) || (x == 6 && y == 1);
                case TerrainClass::Wall:
                    return (x + y) % 2 == 0;
                case TerrainClass::Water:
                    return y % 4 == 2 && x % 4 != 3;
                case TerrainClass::Cliff:
                    return x % 3 != 1;
                case TerrainClass::Path:
                    return (x + y) % 4 == 0;
                case TerrainClass::Stair:
                    return y % 2 == 0;
            }

            return false;
        }

        [[nodiscard]] float coverage(
            const std::uint8_t mask,
            const std::int32_t x,
            const std::int32_t y)
        {
            const auto fx = static_cast<float>(x) / 7.0F;
            const auto fy = static_cast<float>(y) / 7.0F;

            auto value = 0.0F;

            if ((mask & 1) != 0)
            {
                value += (1.0F - fx) * (1.0F - fy);
            }

            if ((mask & 2) != 0)
            {
                value += fx * (1.0F - fy);
            }

            if ((mask & 4) != 0)
            {
                value += (1.0F - fx) * fy;
            }

            if ((mask & 8) != 0)
            {
                value += fx * fy;
            }

            return value;
        }

        [[nodiscard]] bool surfaceInk(
            const TerrainClass terrain,
            const std::uint8_t mask,
            const std::int32_t x,
            const std::int32_t y)
        {
            const auto value = coverage(mask, x, y);

            if (value < 0.5F)
            {
                return false;
            }

            if (value < 0.66F)
            {
                return true;
            }

            return patternInk(terrain, x, y);
        }

        [[nodiscard]] bool bandInk(
            const std::int32_t x, const std::int32_t y)
        {
            return x % 2 == 0 || y % 4 == 0;
        }

        [[nodiscard]] bool rimInk(
            const std::int32_t x, const std::int32_t y)
        {
            return y < 2 || x % 4 == 0;
        }
    }

    Bitmap placeholderSheet(const TerrainClass terrain, const Color ink)
    {
        Bitmap sheet{
            .size = {.width = kSheetWidth, .height = kSheetHeight},
            .pixels = {}};

        sheet.pixels.assign(
            static_cast<std::size_t>(kSheetWidth) * kSheetHeight * 4,
            0);

        const auto put = [&](const std::int32_t x, const std::int32_t y)
        {
            const auto offset =
                (static_cast<std::size_t>(y) * kSheetWidth
                 + static_cast<std::size_t>(x))
                * 4;

            sheet.pixels[offset] = ink.red;
            sheet.pixels[offset + 1] = ink.green;
            sheet.pixels[offset + 2] = ink.blue;
            sheet.pixels[offset + 3] = 255;
        };

        for (std::uint8_t mask = 0; mask < 16; ++mask)
        {
            const auto originX = (mask % 4) * 8;
            const auto originY = (mask / 4) * 8;

            for (std::int32_t y = 0; y < 8; ++y)
            {
                for (std::int32_t x = 0; x < 8; ++x)
                {
                    if (surfaceInk(terrain, mask, x, y))
                    {
                        put(originX + x, originY + y);
                    }
                }
            }
        }

        for (std::int32_t y = 0; y < 8; ++y)
        {
            for (std::int32_t x = 0; x < 8; ++x)
            {
                if (bandInk(x, y))
                {
                    put(x, 32 + y);
                }

                if (rimInk(x, y))
                {
                    put(8 + x, 32 + y);
                }
            }
        }

        return sheet;
    }

}
