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

    [[nodiscard]] Color colorOf(Terrain terrain) noexcept;

    struct WorldMapSnapshot final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<Terrain> tiles;
        std::array<Cell, kCityCount> cities{};

        [[nodiscard]] bool operator==(
            const WorldMapSnapshot &other) const = default;
    };

    [[nodiscard]] WorldMapSnapshot worldSnapshotOf(const WorldMap &world);

    class WorldMapScene final
    {
    public:
        void draw(
            IRenderer &renderer,
            Size canvas,
            const WorldMapSnapshot &snapshot) const;
    };

}
