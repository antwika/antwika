#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/wfc/Domain.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Terrain.hpp"

namespace antwika::game
{

    inline constexpr std::size_t kCityCount = 4;

    struct WorldMapConfig final
    {
        std::uint32_t width = 24;
        std::uint32_t height = 16;
        std::uint64_t seed = 0;

        [[nodiscard]] bool operator==(
            const WorldMapConfig &other) const = default;
    };

    struct WorldMap final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        std::vector<Terrain> tiles;

        std::array<std::size_t, kCityCount> cities{};

        [[nodiscard]] Terrain at(
            std::uint32_t x, std::uint32_t y) const;

        [[nodiscard]] Cell cityCell(std::size_t city) const;

        [[nodiscard]] std::size_t cityAt(Cell cell) const;

        [[nodiscard]] bool operator==(const WorldMap &other) const = default;
    };

    [[nodiscard]] std::vector<antwika::wfc::Domain> buildWorldWave(
        const WorldMapConfig &config);

    [[nodiscard]] std::vector<Terrain> solveTerrain(
        std::uint32_t width,
        std::uint32_t height,
        std::vector<antwika::wfc::Domain> wave);

    [[nodiscard]] std::array<std::size_t, kCityCount> placeCities(
        std::uint32_t width,
        std::uint32_t height,
        const std::vector<Terrain> &tiles);

    [[nodiscard]] WorldMap generateWorldMap(const WorldMapConfig &config);

}
