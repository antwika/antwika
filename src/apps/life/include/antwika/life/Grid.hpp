#pragma once

#include <cstdint>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

namespace antwika::life
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;

    class Grid final
    {
    public:
        Grid(World &world, std::uint32_t width, std::uint32_t height);

        [[nodiscard]] std::uint32_t width() const noexcept;

        [[nodiscard]] std::uint32_t height() const noexcept;

        [[nodiscard]] bool contains(
            std::uint32_t x, std::uint32_t y) const noexcept;

        [[nodiscard]] Entity entityAt(std::uint32_t x, std::uint32_t y) const;

    private:
        std::uint32_t widthValue;
        std::uint32_t heightValue;
        std::vector<Entity> entities;
    };

}
