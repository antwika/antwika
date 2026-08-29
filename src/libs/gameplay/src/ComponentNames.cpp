#include "antwika/gameplay/ComponentNames.hpp"

#include <algorithm>
#include <array>
#include <tuple>
#include <type_traits>
#include <variant>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/CheckpointReport.hpp>
#include <antwika/component/ConsumeReport.hpp>
#include <antwika/component/ExitReport.hpp>
#include <antwika/component/DialogueLine.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
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

        using ModuleComponents = std::tuple<
            component::AnimationState,
            component::CarriedLight,
            component::CheckpointReport,
            component::ConsumeReport,
            component::DialogueLine,
            component::ExitReport,
            component::FillLight,
            component::Health,
            component::Inventory,
            component::Item,
            component::Orientation,
            component::Pad,
            component::Patrol,
            component::Player,
            component::Position,
            component::CharacterIndex,
            component::Speaker,
            component::Velocity>;

        template <typename T, typename Held>
        struct HeldBy;

        template <typename T, typename... Ones>
        struct HeldBy<T, std::tuple<Ones...>>
            : std::bool_constant<(std::is_same_v<T, Ones> || ...)>
        {
        };

        struct NamedComponent final
        {
            std::string_view name;
            Adder adder;
        };

        template <typename T>
        [[nodiscard]] constexpr NamedComponent named(
            const std::string_view name, const Adder adder)
        {
            static_assert(
                HeldBy<T, ModuleComponents>::value,
                "every component the module can spawn by name must "
                "be in ModuleComponents so its pool is claimed "
                "host-side before the module loads");

            return NamedComponent{.name = name, .adder = adder};
        }

        template <typename... Ones>
        void getClaimed(
            ecs::World &world, std::type_identity<std::tuple<Ones...>>)
        {
            (world.claim<Ones>(), ...);
        }

        template <std::size_t N>
        struct SpawnName final
        {
            std::array<char, N> letters{};

            constexpr SpawnName(const char (&text)[N])
            {
                std::ranges::copy(text, letters.begin());
            }

            [[nodiscard]] constexpr std::string_view toView() const
            {
                return std::string_view{letters.data(), N - 1};
            }
        };

        template <typename T>
        [[nodiscard]] const T *getValued(
            const SpawnContext &spawnContext,
            const std::string_view name)
        {
            if (spawnContext.componentValues == nullptr)
            {
                return nullptr;
            }

            const auto foundEntry = spawnContext.componentValues->find(name);

            return foundEntry == spawnContext.componentValues->end()
                       ? nullptr
                       : &std::get<T>(foundEntry->second);
        }

        template <typename T, SpawnName kName>
        [[nodiscard]] constexpr NamedComponent valued()
        {
            return named<T>(
                kName.toView(),
                [](ecs::World &world,
                   const ecs::Entity entity,
                   const SpawnContext &spawnContext)
                {
                    const auto *const givenValue =
                        getValued<T>(spawnContext, kName.toView());

                    world.add<T>(
                        entity,
                        givenValue == nullptr ? T{} : *givenValue);
                });
        }

        constexpr std::array kNamedComponents{
            named<component::Position>(
                "component::Position",
                [](ecs::World &world,
                   const ecs::Entity entity,
                   const SpawnContext &spawnContext)
                {
                    world.add<component::Position>(
                        entity,
                        collision::positionFrom(
                            spawnContext.placement.position));
                }),
            named<component::AnimationState>(
                "component::AnimationState",
                [](ecs::World &world,
                   const ecs::Entity entity,
                   const SpawnContext &spawnContext)
                {
                    world.add<component::AnimationState>(
                        entity,
                        component::AnimationState{
                            .direction = spawnContext.placement.way});
                }),
            named<component::CharacterIndex>(
                "component::CharacterIndex",
                [](ecs::World &world,
                   const ecs::Entity entity,
                   const SpawnContext &spawnContext)
                {
                    world.add<component::CharacterIndex>(
                        entity,
                        component::CharacterIndex{
                            .index = spawnContext.index});
                }),
            named<component::Inventory>(
                "component::Inventory",
                [](ecs::World &world,
                   const ecs::Entity entity,
                   const SpawnContext &spawnContext)
                {
                    const auto *const givenValue =
                        getValued<component::Inventory>(
                            spawnContext, "component::Inventory");

                    world.add<component::Inventory>(
                        entity,
                        givenValue == nullptr
                            ? rules::getStartingInventory()
                            : *givenValue);
                }),
            valued<component::Velocity, "component::Velocity">(),
            valued<component::Player, "component::Player">(),
            valued<component::Health, "component::Health">(),
            valued<component::Patrol, "component::Patrol">(),
            valued<component::Speaker, "component::Speaker">(),
            valued<component::CarriedLight, "component::CarriedLight">(),
            valued<component::FillLight, "component::FillLight">()};

        constexpr std::array<std::string_view, 9> kPlayerComponentNames{
            "component::Position",
            "component::Velocity",
            "component::AnimationState",
            "component::CharacterIndex",
            "component::Health",
            "component::Inventory",
            "component::Player",
            "component::FillLight",
            "component::CarriedLight"};

        constexpr std::array<std::string_view, 5> kCharacterComponentNames{
            "component::Position",
            "component::Velocity",
            "component::AnimationState",
            "component::CharacterIndex",
            "component::Health"};

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

    std::span<const std::string_view> getPlayerComponentNames()
    {
        return kPlayerComponentNames;
    }

    std::span<const std::string_view> getCharacterComponentNames()
    {
        return kCharacterComponentNames;
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

    void claimModuleComponents(ecs::World &world)
    {
        getClaimed(world, std::type_identity<ModuleComponents>{});
    }

}
