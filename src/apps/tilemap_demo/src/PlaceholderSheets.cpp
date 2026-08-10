#include "antwika/tilemap_demo/PlaceholderSheets.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/autotile/SheetLayout.hpp>

namespace antwika::tilemap_demo
{

    namespace
    {
        using antwika::autotile::kDisplayTile;
        using antwika::autotile::kSheetHeight;
        using antwika::autotile::kSheetWidth;
        using antwika::gfx::Bitmap;
        using antwika::gfx::Color;
        using antwika::tilemap::TerrainClass;

        constexpr std::int32_t kRightColumn = 64;

        constexpr std::int32_t kSpecialRow = 32;

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
            const auto fx = static_cast<float>(x) / 15.0F;
            const auto fy = static_cast<float>(y) / 15.0F;

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

            return patternInk(terrain, x % 8, y % 8);
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

        [[nodiscard]] bool bridgeInk(
            const std::int32_t x, const std::int32_t y)
        {
            return y % 4 != 0 && (x + 2 * (y / 4)) % 8 != 7;
        }

        [[nodiscard]] bool shadeInk(
            const std::int32_t x, const std::int32_t y)
        {
            return (x + y) % 2 == 0;
        }

        [[nodiscard]] bool variantOneInk(
            const TerrainClass terrain,
            const std::int32_t x,
            const std::int32_t y)
        {
            return patternInk(terrain, (x + 3) % 8, y % 8);
        }

        [[nodiscard]] bool variantTwoInk(
            const TerrainClass terrain,
            const std::int32_t x,
            const std::int32_t y)
        {
            return patternInk(terrain, x % 8, (y + 3) % 8);
        }

        [[nodiscard]] bool frameBInk(
            const TerrainClass terrain,
            const std::int32_t x,
            const std::int32_t y)
        {
            return patternInk(terrain, (x + 2) % 8, y % 8);
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
            const auto originX = (mask % 4) * kDisplayTile;
            const auto originY = (mask / 4) * kDisplayTile;

            for (std::int32_t y = 0; y < kDisplayTile; ++y)
            {
                for (std::int32_t x = 0; x < kDisplayTile; ++x)
                {
                    if (surfaceInk(terrain, mask, x, y))
                    {
                        put(originX + x, originY + y);
                    }
                }
            }
        }

        for (std::int32_t y = 0; y < kDisplayTile; ++y)
        {
            for (std::int32_t x = 0; x < kDisplayTile; ++x)
            {
                if (variantOneInk(terrain, x, y))
                {
                    put(kRightColumn + x, y);
                }

                if (variantTwoInk(terrain, x, y))
                {
                    put(kRightColumn + kDisplayTile + x, y);
                }

                if (frameBInk(terrain, x, y))
                {
                    put(kRightColumn + x, kDisplayTile + y);
                }
            }
        }

        for (std::int32_t y = 0; y < 8; ++y)
        {
            for (std::int32_t x = 0; x < 8; ++x)
            {
                if (bandInk(x, y))
                {
                    put(kRightColumn + x, kSpecialRow + y);
                }

                if (rimInk(x, y))
                {
                    put(kRightColumn + 8 + x, kSpecialRow + y);
                }

                if (bridgeInk(x, y))
                {
                    put(kRightColumn + 16 + x, kSpecialRow + y);
                }

                if (shadeInk(x, y))
                {
                    put(kRightColumn + 24 + x, kSpecialRow + y);
                }
            }
        }

        return sheet;
    }

}
