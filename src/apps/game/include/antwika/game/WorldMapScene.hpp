#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Terrain.hpp"
#include "antwika/game/WorldMap.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    /**
     * @brief What one colour a terrain is drawn in.
     * @param terrain The terrain to colour.
     * @return Its colour; anything unrecognised comes back as water,
     * which cannot happen through terrainOf() but keeps the function
     * total.
     */
    [[nodiscard]] Color colorOf(Terrain terrain) noexcept;

    /**
     * @brief Everything a world-map frame needs, and nothing that can
     * change under it.
     *
     * The same plain-value shape as SceneSnapshot and
     * poker::TableSnapshot, and for the same reason: it makes
     * WorldMapScene a pure function of its argument, so the picture
     * can be asserted call by call against a mock renderer rather than
     * having to be looked at.
     */
    struct WorldMapSnapshot
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<Terrain> tiles;
        std::array<Cell, kCityCount> cities{};

        /**
         * @brief Compare two snapshots.
         * @param other The snapshot to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const WorldMapSnapshot &other) const = default;
    };

    /**
     * @brief Take a snapshot of the world map.
     * @param world The map to describe.
     * @return The frame's description.
     */
    [[nodiscard]] WorldMapSnapshot worldSnapshotOf(const WorldMap &world);

    /**
     * @brief Draws the world map: its terrain, and its four cities.
     *
     * Stateless and deterministic, like GridScene: the same snapshot
     * and canvas always produce the same drawing calls in the same
     * order. It reads nothing back, and nothing it draws reaches the
     * simulation -- which map is showing and which city was picked are
     * both decided in the tick path, by WorldMapSink.
     *
     * Where a tile is drawn comes from WorldMapLayout, the same header
     * WorldMapSink resolves a click through, so the picture and the
     * hit test cannot drift apart.
     */
    class WorldMapScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size the map is centred in; it must be the
         * size the window was asked for, matching what WorldMapSink is
         * given.
         * @param snapshot What to draw.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const WorldMapSnapshot &snapshot) const;
    };

} // namespace antwika::game
