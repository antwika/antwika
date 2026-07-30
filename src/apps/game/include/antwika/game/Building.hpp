#pragma once

#include <cstdint>

#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief What a building is for.
     */
    enum class BuildingKind : std::uint8_t
    {
        House,          ///< Consumes what walkers bring it.
        FoodSource,     ///< Spawns a food walker.
        WaterSource,    ///< Spawns a water walker.
        FireStation,    ///< Spawns a fireman.
        ArchitectPost,  ///< Spawns an architect.
    };

    /**
     * @brief A building's stock of one resource.
     */
    struct Stock
    {
        Resource resource = Resource::Food;
        std::int32_t held = 0;
        std::int32_t capacity = 100;

        [[nodiscard]] bool operator==(const Stock &other) const = default;
    };

} // namespace antwika::game
