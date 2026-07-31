#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::game
{

    /**
     * @brief What one world-map tile is made of.
     *
     * The order is an elevation ladder, and that is load-bearing rather
     * than cosmetic: the world map's only adjacency rule is that two
     * neighbouring tiles differ by at most one step along it. So water
     * can touch plains and plains can touch forest, but water can never
     * touch mountain, which is what stops a lone sea tile turning up in
     * the middle of a range.
     *
     * Fixed width and explicit values, because a tile's value is the
     * symbol index antwika::wfc solves over -- Water is alphabet index
     * 0, Mountain is index 4 -- and the two must not be able to drift.
     */
    enum class Terrain : std::uint8_t
    {
        Water = 0,
        Plains = 1,
        Forest = 2,
        Hills = 3,
        Mountain = 4,
    };

    /**
     * @brief How many terrains there are, i.e. the solver's alphabet
     * size.
     */
    inline constexpr std::size_t kTerrainCount = 5;

    /**
     * @brief Check whether a tile can be built on.
     * @param terrain The tile to ask about.
     * @return True for anything that is not water.
     */
    [[nodiscard]] constexpr bool isLand(Terrain terrain) noexcept
    {
        return terrain != Terrain::Water;
    }

    /**
     * @brief Turn a solver symbol index into a terrain.
     * @param symbol An index in [0, kTerrainCount); anything larger is
     * clamped to Mountain.
     * @return The terrain that symbol stands for.
     */
    [[nodiscard]] constexpr Terrain terrainOf(std::size_t symbol) noexcept
    {
        return symbol < kTerrainCount
                   ? static_cast<Terrain>(symbol)
                   : Terrain::Mountain;
    }

    /**
     * @brief Turn a terrain into the solver symbol index for it.
     * @param terrain The terrain to convert.
     * @return Its index into the solver's alphabet.
     */
    [[nodiscard]] constexpr std::size_t symbolOf(Terrain terrain) noexcept
    {
        return static_cast<std::size_t>(terrain);
    }

} // namespace antwika::game
