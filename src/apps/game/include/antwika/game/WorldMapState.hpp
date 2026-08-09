#pragma once

#include <array>
#include <cstddef>

#include "antwika/game/Camera.hpp"
#include "antwika/game/CityGrid.hpp"
#include "antwika/game/LiveGrid.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/WorldMap.hpp"

namespace antwika::game
{

    class WorldMapState final
    {
    public:
        explicit WorldMapState(WorldMap world);

        [[nodiscard]] const WorldMap &world() const noexcept;

        [[nodiscard]] bool cityOpen() const noexcept;

        [[nodiscard]] std::size_t city() const noexcept;

        void openCityAt(std::size_t index, const LiveGrid &live);

        void closeCity(const LiveGrid &live);

        [[nodiscard]] PathIndex &cityPaths(std::size_t city);

        [[nodiscard]] const PathIndex &cityPaths(std::size_t city) const;

        [[nodiscard]] Camera &cityCamera(std::size_t city);

        [[nodiscard]] const Camera &cityCamera(std::size_t city) const;

    private:
        void requireCity(std::size_t city) const;

        WorldMap map;
        bool open = true;
        std::size_t liveCity = 0;
        std::array<PathIndex, kCityCount> paths{};
        std::array<CityGrid, kCityCount> grids{};
        std::array<Camera, kCityCount> cameras{
            Camera{}, Camera{}, Camera{}, Camera{}};
    };

}
