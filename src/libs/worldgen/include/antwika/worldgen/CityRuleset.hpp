#pragma once

#include <cstddef>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/worldgen/Ruleset.hpp"

namespace antwika::worldgen
{

    enum class CityPiece : std::size_t
    {
        AirOpen,
        AirRoom,
        Bedrock,
        Fill,
        Wall,
        Floor,
        CorbelEast,
        CorbelWest,
        CorbelNorth,
        CorbelSouth,
        StairEast,
        StairWest,
        StairNorth,
        StairSouth,
        Cistern,
    };

    [[nodiscard]] constexpr CityPiece getLastEnumerator(CityPiece) noexcept
    {
        return CityPiece::Cistern;
    }

    inline constexpr std::size_t kCityPieces =
        antwika::enums::kCount<CityPiece>;

    [[nodiscard]] std::size_t indexOf(CityPiece piece);

    enum class CityDistrict : std::size_t
    {
        Bedrock,
        Undercroft,
        Slums,
        Middling,
        Heights,
        Sky,
    };

    [[nodiscard]] Ruleset getCityRuleset();

}
