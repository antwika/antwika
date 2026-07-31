#include "antwika/game/WorldMapScene.hpp"

#include <string>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/WorldMapLayout.hpp"

namespace antwika::game
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    namespace
    {
        constexpr Color kWater{40, 70, 130, 255};
        constexpr Color kPlains{110, 160, 70, 255};
        constexpr Color kForest{50, 110, 55, 255};
        constexpr Color kHills{140, 125, 80, 255};
        constexpr Color kMountain{130, 130, 135, 255};

        constexpr Color kCityFill{230, 210, 120, 255};
        constexpr Color kCityLabel{30, 25, 20, 255};

        // A city marker is a third of a tile, centred, so the terrain
        // under it stays readable.
        constexpr std::int32_t kMarkerInset = kWorldTileSize / 3;
    } // namespace

    Color colorOf(Terrain terrain) noexcept
    {
        switch (terrain)
        {
        case Terrain::Plains:
            return kPlains;
        case Terrain::Forest:
            return kForest;
        case Terrain::Hills:
            return kHills;
        case Terrain::Mountain:
            return kMountain;
        case Terrain::Water:
            break;
        }
        return kWater;
    }

    WorldMapSnapshot worldSnapshotOf(const WorldMap &world)
    {
        WorldMapSnapshot snapshot;
        snapshot.width = world.width;
        snapshot.height = world.height;
        snapshot.tiles = world.tiles;
        for (std::size_t city = 0; city < kCityCount; ++city)
        {
            snapshot.cities[city] = world.cityCell(city);
        }
        return snapshot;
    } // GCOVR_EXCL_LINE

    void WorldMapScene::draw(
        IRenderer &renderer,
        Size canvas,
        const WorldMapSnapshot &snapshot) const
    {
        for (std::uint32_t y = 0; y < snapshot.height; ++y)
        {
            for (std::uint32_t x = 0; x < snapshot.width; ++x)
            {
                const std::size_t index =
                    static_cast<std::size_t>(y) * snapshot.width + x;
                const Cell cell{
                    static_cast<std::int32_t>(x),
                    static_cast<std::int32_t>(y)};
                renderer.drawRect(
                    worldTileRect(
                        canvas, snapshot.width, snapshot.height, cell),
                    colorOf(snapshot.tiles[index]));
            }
        }

        for (std::size_t city = 0; city < kCityCount; ++city)
        {
            const Rect tile = worldTileRect(
                canvas,
                snapshot.width,
                snapshot.height,
                snapshot.cities[city]);
            const Point origin{
                tile.origin.x + kMarkerInset,
                tile.origin.y + kMarkerInset};
            const auto side = static_cast<std::uint32_t>(
                kWorldTileSize - 2 * kMarkerInset);
            renderer.drawRect(Rect{origin, Size{side, side}}, kCityFill);
            renderer.drawText(
                origin, std::to_string(city + 1), 1, kCityLabel);
        }
    }

} // namespace antwika::game
