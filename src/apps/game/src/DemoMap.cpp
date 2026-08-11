#include "antwika/game/DemoMap.hpp"

#include <cstdint>
#include <utility>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

namespace antwika::game
{

    namespace
    {
        using antwika::geometry::GridCell;
        using antwika::tilemap::Slab;
        using antwika::tilemap::TerrainClass;
        using antwika::tilemap::TileMap;

        constexpr std::uint32_t kColumns = 20;
        constexpr std::uint32_t kRows = 11;

        void paint(
            TileMap &map,
            const std::uint32_t left,
            const std::uint32_t top,
            const std::uint32_t right,
            const std::uint32_t bottom,
            const TerrainClass terrain,
            const std::int32_t height)
        {
            for (auto row = top; row <= bottom; ++row)
            {
                for (auto column = left; column <= right; ++column)
                {
                    auto &stack = map.at(
                        GridCell{.column = column, .row = row});

                    stack.clear();

                    for (std::int32_t level = 0; level <= height;
                         ++level)
                    {
                        stack.place(Slab{
                            .level = level, .terrain = terrain});
                    }
                }
            }
        }
    }

    TileMap demoMap()
    {
        antwika::tilemap::MapHeader header;
        header.id = "wakewater-demo";
        header.ink = antwika::tilemap::Rgb{
            .red = 214, .green = 224, .blue = 216};
        header.paper = antwika::tilemap::Rgb{
            .red = 12, .green = 14, .blue = 16};

        TileMap map(std::move(header), kColumns, kRows);

        paint(map, 0, 0, kColumns - 1, 0, TerrainClass::Wall, 0);
        paint(map, 0, kRows - 1, kColumns - 1, kRows - 1,
              TerrainClass::Wall, 0);
        paint(map, 0, 0, 0, kRows - 1, TerrainClass::Wall, 0);
        paint(map, kColumns - 1, 0, kColumns - 1, kRows - 1,
              TerrainClass::Wall, 0);

        paint(map, 1, 5, 18, 5, TerrainClass::Path, 0);

        paint(map, 2, 6, 6, 9, TerrainClass::Water, 0);

        paint(map, 3, 2, 5, 3, TerrainClass::Floor, 1);

        paint(map, 13, 1, 18, 4, TerrainClass::Floor, 2);
        paint(map, 14, 5, 14, 5, TerrainClass::Stair, 1);

        paint(map, 7, 2, 10, 4, TerrainClass::Wall, 4);

        return map;
    } // GCOVR_EXCL_LINE

}
