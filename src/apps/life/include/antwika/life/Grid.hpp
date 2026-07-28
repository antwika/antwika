#pragma once

#include <cstdint>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

namespace antwika::life
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;

    /**
     * @brief Maps a fixed-size grid of (x, y) coordinates to ECS entities.
     *
     * Creates one entity per cell up front, each holding a Cell component
     * staged to {alive = false} -- the caller commits the world once after
     * construction to make that initial state visible.
     */
    class Grid final
    {
    public:
        /**
         * @brief Create width * height entities, one per cell.
         * @param world World the cell entities are created in.
         * @param width Number of columns.
         * @param height Number of rows.
         */
        Grid(World &world, std::uint32_t width, std::uint32_t height);

        /**
         * @brief Get the number of columns.
         */
        [[nodiscard]] std::uint32_t width() const noexcept;

        /**
         * @brief Get the number of rows.
         */
        [[nodiscard]] std::uint32_t height() const noexcept;

        /**
         * @brief Check whether (x, y) is within the grid's bounds.
         */
        [[nodiscard]] bool contains(
            std::uint32_t x, std::uint32_t y) const noexcept;

        /**
         * @brief Get the entity representing the cell at (x, y).
         * @throws std::out_of_range if (x, y) is out of bounds.
         */
        [[nodiscard]] Entity entityAt(std::uint32_t x, std::uint32_t y) const;

    private:
        std::uint32_t widthValue;
        std::uint32_t heightValue;
        std::vector<Entity> entities;
    };

} // namespace antwika::life
