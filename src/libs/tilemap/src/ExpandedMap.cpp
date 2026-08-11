#include "antwika/tilemap/ExpandedMap.hpp"

#include <cstdint>
#include <utility>
#include <variant>

#include <antwika/geometry/Grid.hpp>

#include "antwika/tilemap/Entities.hpp"

namespace antwika::tilemap
{

    TileMap expandedMap(
        const TileMap &map,
        const std::uint32_t west,
        const std::uint32_t north,
        const std::uint32_t east,
        const std::uint32_t south)
    {
        TileMap grown(
            map.header(),
            map.columns() + west + east,
            map.rows() + north + south);

        for (std::uint32_t row = 0; row < map.rows(); ++row)
        {
            for (std::uint32_t column = 0; column < map.columns();
                 ++column)
            {
                grown.at(geometry::GridCell{
                    .column = column + west,
                    .row = row + north}) =
                    map.at(geometry::GridCell{
                        .column = column, .row = row});
            }
        }

        for (const auto &entity : map.entities())
        {
            Entity shifted = entity;

            std::visit(
                [west, north](auto &kind)
                {
                    kind.at.column += west;
                    kind.at.row += north;
                },
                shifted);

            grown.addEntity(std::move(shifted));
        }

        return grown;
    } // GCOVR_EXCL_LINE

}
