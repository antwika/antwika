#include "antwika/tilemap_demo/PlaceholderTilesets.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>

namespace antwika::tilemap_demo
{

    namespace
    {
        using antwika::gfx::Bitmap;
        using antwika::tilemap::TerrainClass;
        using antwika::tileset::kSpriteSide;
        using antwika::tileset::PixelClass;
        using antwika::tileset::Side;
        using antwika::tileset::SocketId;

        struct EdgeSet final
        {
            bool north = false;
            bool east = false;
            bool south = false;
            bool west = false;
        };

        constexpr std::array<EdgeSet, 9> kBorderShapes{
            EdgeSet{},
            EdgeSet{.north = true},
            EdgeSet{.east = true},
            EdgeSet{.south = true},
            EdgeSet{.west = true},
            EdgeSet{.north = true, .west = true},
            EdgeSet{.north = true, .east = true},
            EdgeSet{.south = true, .west = true},
            EdgeSet{.east = true, .south = true}};

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

        [[nodiscard]] bool borderInk(
            const EdgeSet &edges,
            const std::int32_t x,
            const std::int32_t y)
        {
            return (edges.north && y == 0)
                   || (edges.south && y == kSpriteSide - 1)
                   || (edges.west && x == 0)
                   || (edges.east && x == kSpriteSide - 1);
        }

        void paintSprite(
            tileset::Sprite &sprite,
            const TerrainClass terrain,
            const EdgeSet &edges)
        {
            for (std::int32_t y = 0; y < kSpriteSide; ++y)
            {
                for (std::int32_t x = 0; x < kSpriteSide; ++x)
                {
                    auto value = PixelClass::Blank;

                    if (patternInk(terrain, x, y))
                    {
                        value = PixelClass::Paper;
                    }

                    if (borderInk(edges, x, y))
                    {
                        value = PixelClass::Ink;
                    }

                    sprite.frames[0].pixels[static_cast<
                        std::size_t>(y * kSpriteSide + x)] = value;
                }
            }
        }
    }

    tileset::Tileset placeholderTileset(const TerrainClass terrain)
    {
        tileset::Tileset set;

        set.name = "placeholder-"
                   + std::string(tilemap::toString(terrain));
        set.terrain = terrain;

        const auto fill = tileset::internSocket(set, "fill");

        for (const auto &edges : kBorderShapes)
        {
            auto &sprite = tileset::addSprite(set, 0);

            const auto pick = [fill](const bool edge)
            {
                return edge ? tileset::kEdgeSocket : fill;
            };

            sprite.sockets = {
                pick(edges.north),
                pick(edges.east),
                pick(edges.south),
                pick(edges.west)};
            paintSprite(sprite, terrain, edges);
        }

        return set;
    } // GCOVR_EXCL_LINE

    namespace
    {
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
    }

    Bitmap placeholderSystemSheet()
    {
        constexpr std::uint32_t kWidth = 32;
        constexpr std::uint32_t kHeight = 8;

        Bitmap sheet{
            .size = {.width = kWidth, .height = kHeight},
            .pixels = {}};

        sheet.pixels.assign(
            static_cast<std::size_t>(kWidth) * kHeight
                * gfx::kBytesPerPixel,
            0);

        const auto put =
            [&sheet](const std::int32_t x, const std::int32_t y)
        {
            const auto offset =
                (static_cast<std::size_t>(y) * kWidth
                 + static_cast<std::size_t>(x))
                * gfx::kBytesPerPixel;

            sheet.pixels[offset] = 255;
            sheet.pixels[offset + 1] = 255;
            sheet.pixels[offset + 2] = 255;
            sheet.pixels[offset + 3] = 255;
        };

        for (std::int32_t y = 0; y < 8; ++y)
        {
            for (std::int32_t x = 0; x < 8; ++x)
            {
                if (bandInk(x, y))
                {
                    put(x, y);
                }

                if (rimInk(x, y))
                {
                    put(8 + x, y);
                }

                if (bridgeInk(x, y))
                {
                    put(16 + x, y);
                }

                if (shadeInk(x, y))
                {
                    put(24 + x, y);
                }
            }
        }

        return sheet;
    } // GCOVR_EXCL_LINE

}
