#include "antwika/tilemap_demo/PlaceholderSheets.hpp"

#include <array>
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

        constexpr std::int32_t kSpecialRow = 64;

        struct Shift final
        {
            std::int32_t x = 0;
            std::int32_t y = 0;
        };

        constexpr std::array<Shift, 7> kVariantShifts{
            Shift{.x = 3, .y = 0},
            Shift{.x = 0, .y = 3},
            Shift{.x = 2, .y = 5},
            Shift{.x = 5, .y = 2},
            Shift{.x = 1, .y = 6},
            Shift{.x = 6, .y = 1},
            Shift{.x = 4, .y = 4}};

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

        [[nodiscard]] bool onPipe(const std::int32_t at)
        {
            return at == 7 || at == 8;
        }

        [[nodiscard]] bool pipeInk(
            const std::int32_t piece,
            const std::int32_t x,
            const std::int32_t y)
        {
            switch (piece)
            {
                case 0:
                    return onPipe(x) || onPipe(y);
                case 1:
                    return onPipe(y);
                case 2:
                    return onPipe(x);
                case 3:
                    return (onPipe(x) && y <= 8)
                           || (onPipe(y) && x >= 7);
                case 4:
                    return (onPipe(x) && y >= 7)
                           || (onPipe(y) && x <= 8);
                case 5:
                    return onPipe(y) || (onPipe(x) && y >= 7);
                case 6:
                    return onPipe(y)
                           || (x >= 5 && x <= 10 && y >= 5
                               && y <= 10
                               && (x == 5 || x == 10 || y == 5
                                   || y == 10));
                default:
                    return (x >= 3 && x <= 12 && y >= 3 && y <= 12
                            && (x == 3 || x == 12 || y == 3
                                || y == 12))
                           || (onPipe(x) && (y < 3 || y > 12))
                           || (onPipe(y) && (x < 3 || x > 12))
                           || ((y == 6 || y == 9) && x >= 5
                               && x <= 10);
            }
        }

        [[nodiscard]] bool wallBackdropInk(
            const std::int32_t x, const std::int32_t y)
        {
            return x % 4 == 2 && y % 4 == 2;
        }

        [[nodiscard]] bool wallInteriorInk(
            const std::int32_t piece,
            const std::int32_t x,
            const std::int32_t y)
        {
            return wallBackdropInk(x, y) || pipeInk(piece, x, y);
        }

        [[nodiscard]] bool floorDetailInk(
            const std::int32_t variant,
            const std::int32_t x,
            const std::int32_t y)
        {
            switch (variant)
            {
                case 1:
                    return y == 8 && x % 3 != 2;
                case 2:
                    return x == 8 && y % 3 != 2;
                case 3:
                    return (x == 2 || x == 13)
                           && (y == 2 || y == 13);
                case 4:
                    return x >= 5 && x <= 10 && y >= 5 && y <= 10
                           && (x + y) % 2 == 0;
                case 5:
                    return (x == 3 && y == 10)
                           || (x == 11 && y == 4)
                           || (x == 12 && y == 12);
                case 6:
                    return (x == 8 && y <= 8 && y % 3 != 2)
                           || (y == 8 && x <= 8 && x % 3 != 2);
                default:
                    return x >= 6 && x <= 9 && y >= 6 && y <= 9
                           && (x == 6 || x == 9 || y == 6
                               || y == 9);
            }
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
            if (mask == 15 && terrain == TerrainClass::Wall)
            {
                return wallInteriorInk(0, x, y);
            }

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

        [[nodiscard]] bool variantInk(
            const TerrainClass terrain,
            const std::int32_t variant,
            const std::int32_t x,
            const std::int32_t y)
        {
            if (terrain == TerrainClass::Wall)
            {
                return wallInteriorInk(variant, x, y);
            }

            if (terrain == TerrainClass::Floor)
            {
                return patternInk(terrain, x % 8, y % 8)
                       || floorDetailInk(variant, x, y);
            }

            const auto &shift = kVariantShifts[static_cast<
                std::size_t>(variant - 1)];

            return patternInk(
                terrain,
                (x + shift.x) % 8,
                (y + shift.y) % 8);
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

        for (std::int32_t variant = 1; variant <= 7; ++variant)
        {
            const auto slot = variant - 1;
            const auto originX =
                kRightColumn + slot % 2 * kDisplayTile;
            const auto originY = slot / 2 * kDisplayTile;

            for (std::int32_t y = 0; y < kDisplayTile; ++y)
            {
                for (std::int32_t x = 0; x < kDisplayTile; ++x)
                {
                    if (variantInk(terrain, variant, x, y))
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
                if (frameBInk(terrain, x, y))
                {
                    put(
                        kRightColumn + kDisplayTile + x,
                        3 * kDisplayTile + y);
                }
            }
        }

        for (std::int32_t y = 0; y < 8; ++y)
        {
            for (std::int32_t x = 0; x < 8; ++x)
            {
                if (bandInk(x, y))
                {
                    put(x, kSpecialRow + y);
                }

                if (rimInk(x, y))
                {
                    put(8 + x, kSpecialRow + y);
                }

                if (bridgeInk(x, y))
                {
                    put(16 + x, kSpecialRow + y);
                }

                if (shadeInk(x, y))
                {
                    put(24 + x, kSpecialRow + y);
                }
            }
        }

        return sheet;
    }

}
