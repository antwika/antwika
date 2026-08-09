#pragma once

#include <cstdint>
#include <map>
#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class PopulationSystem final : public ISystem
    {
    public:
        PopulationSystem(
            const PathIndex &paths,
            const BuildingIndex &built,
            const DesirabilityField &desirability,
            GridExtent extent,
            GameConfig config) noexcept;

        PopulationSystem(const PopulationSystem &) = delete;
        PopulationSystem(PopulationSystem &&) = delete;

        PopulationSystem &operator=(const PopulationSystem &) = delete;
        PopulationSystem &operator=(PopulationSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        struct Standing final
        {
            std::int32_t capacity = 0;

            bool pleasant = false;

            bool unlivable = false;

            std::optional<Cell> door{};
        };

        [[nodiscard]] std::map<antwika::ecs::Entity, std::int32_t> admit(
            World &world);

        [[nodiscard]] static std::map<antwika::ecs::Entity, std::int32_t>
        expecting(const World &world);

        [[nodiscard]] Standing standingOf(
            World &world,
            antwika::ecs::Entity entity,
            const Building &building,
            const Household &household);

        void settle(
            World &world,
            antwika::ecs::Entity entity,
            const Building &building,
            Household &household);

        void recruit(
            World &world,
            antwika::ecs::Entity entity,
            const Building &building,
            const Household &household,
            std::int32_t coming);

        void send(
            World &world,
            antwika::ecs::Entity entity,
            Cell door,
            std::int32_t carried);

        void turnOut(
            World &world,
            antwika::ecs::Entity entity,
            std::optional<Cell> door,
            bool emigrates);

        const PathIndex &paths;
        const BuildingIndex &built;
        const DesirabilityField &desirability;
        GridExtent extent;
        GameConfig config;
    };

}
