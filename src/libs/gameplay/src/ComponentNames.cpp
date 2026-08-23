#include "antwika/gameplay/ComponentNames.hpp"

#include <algorithm>
#include <array>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/rules/Items.hpp"

namespace antwika::gameplay
{

    namespace
    {
        using Adder =
            void (*)(ecs::World &, ecs::Entity, const SpawnContext &);

        using Claimer = void (*)(ecs::World &);

        struct NamedComponent final
        {
            std::string_view name;
            Adder adder;
            Claimer claimer;
        };

        template <typename T>
        [[nodiscard]] constexpr Claimer claimerFor()
        {
            return [](ecs::World &world) { world.claim<T>(); };
        }

        template <typename T>
        [[nodiscard]] constexpr NamedComponent plain(
            const std::string_view name)
        {
            return NamedComponent{
                .name = name,
                .adder =
                    [](ecs::World &world,
                       const ecs::Entity entity,
                       const SpawnContext &)
                { world.add<T>(entity, T{}); },
                .claimer = claimerFor<T>()};
        }

        constexpr std::array kNamedComponents{
            NamedComponent{
                .name = "component::Position",
                .adder =
                    [](ecs::World &world,
                       const ecs::Entity entity,
                       const SpawnContext &spawnContext)
                {
                    world.add<component::Position>(
                        entity,
                        collision::positionFrom(
                            spawnContext.placement.position));
                },
                .claimer = claimerFor<component::Position>()},
            NamedComponent{
                .name = "component::AnimationState",
                .adder =
                    [](ecs::World &world,
                       const ecs::Entity entity,
                       const SpawnContext &spawnContext)
                {
                    world.add<component::AnimationState>(
                        entity,
                        component::AnimationState{
                            .direction = spawnContext.placement.way});
                },
                .claimer = claimerFor<component::AnimationState>()},
            NamedComponent{
                .name = "component::RosterIndex",
                .adder =
                    [](ecs::World &world,
                       const ecs::Entity entity,
                       const SpawnContext &spawnContext)
                {
                    world.add<component::RosterIndex>(
                        entity,
                        component::RosterIndex{
                            .index = spawnContext.index});
                },
                .claimer = claimerFor<component::RosterIndex>()},
            NamedComponent{
                .name = "component::Inventory",
                .adder =
                    [](ecs::World &world,
                       const ecs::Entity entity,
                       const SpawnContext &)
                {
                    world.add<component::Inventory>(
                        entity, rules::getStartingInventory());
                },
                .claimer = claimerFor<component::Inventory>()},
            plain<component::Velocity>("component::Velocity"),
            plain<component::Player>("component::Player"),
            plain<component::Health>("component::Health"),
            plain<component::Patrol>("component::Patrol"),
            plain<component::Speaker>("component::Speaker"),
            plain<component::CarriedLight>("component::CarriedLight"),
            plain<component::FillLight>("component::FillLight")};

        [[nodiscard]] const NamedComponent *getLookedUp(
            const std::string_view name)
        {
            const auto foundEntry = std::ranges::find(
                kNamedComponents, name, &NamedComponent::name);

            return foundEntry == kNamedComponents.end() ? nullptr
                                                        : &*foundEntry;
        }
    }

    std::span<const std::string_view> getComponentNames()
    {
        static const std::vector<std::string_view> names = []
        {
            std::vector<std::string_view> gatheredNames;

            for (const auto &one : kNamedComponents)
            {
                gatheredNames.push_back(one.name);
            }

            return gatheredNames;
        }();

        return names;
    }

    bool isComponentNamed(const std::string_view name)
    {
        return getLookedUp(name) != nullptr;
    }

    void addComponentsNamed(
        ecs::World &world,
        const ecs::Entity entity,
        const SpawnContext &spawnContext,
        const std::span<const std::string> names)
    {
        for (const auto &name : names)
        {
            const auto *const one = getLookedUp(name);

            if (one == nullptr)
            {
                throw map::MapFileError(
                    "antwika::gameplay: no component is named \"" + name
                    + "\"");
            }

            one->adder(world, entity, spawnContext);
        }
    }

    void claimNamedComponents(ecs::World &world)
    {
        for (const auto &one : kNamedComponents)
        {
            one.claimer(world);
        }
    }

}
