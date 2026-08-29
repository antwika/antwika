#include "antwika/loadout/Descriptors.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/loadout/ComponentRow.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/loadout/Role.hpp>

#include "DescriptorMakers.hpp"

namespace antwika::loadout
{

    namespace
    {
        using component::AnimationState;
        using component::CarriedLight;
        using component::FillLight;
        using component::Health;
        using component::Inventory;
        using component::Patrol;
        using component::Player;
        using component::Position;
        using component::CharacterIndex;
        using component::Speaker;
        using component::Velocity;

        constexpr double kMostCoord = 4096.0;

        constexpr double kMostVelocity = 32.0;

        constexpr double kMostSpeed = 4.0;

        constexpr std::int64_t kMostWay = 7;

        constexpr std::int64_t kMostTick = 9007199254740992;

        constexpr std::int64_t kMostIndex = 4294967295;

        constexpr double kMostAbove = 8.0;

        constexpr double kMostReach = 64.0;

        constexpr std::array kPositionFields{
            fixedField<
                Position, &Position::x, -kMostCoord, kMostCoord>("x"),
            fixedField<
                Position, &Position::y, -kMostCoord, kMostCoord>("y"),
            fixedField<
                Position, &Position::z, -kMostCoord, kMostCoord>("z")};

        constexpr std::array kVelocityFields{
            fixedField<
                Velocity,
                &Velocity::velocityX,
                -kMostVelocity,
                kMostVelocity>("x"),
            fixedField<
                Velocity,
                &Velocity::velocityZ,
                -kMostVelocity,
                kMostVelocity>("z"),
            fixedField<
                Velocity,
                &Velocity::speedMultiplier,
                0.0,
                kMostSpeed>("speed")};

        constexpr std::array kAnimationStateFields{
            wholeField<
                AnimationState,
                &AnimationState::direction,
                0,
                kMostWay>("way"),
            flagField<
                AnimationState, &AnimationState::walking>("walking"),
            wholeField<
                AnimationState,
                &AnimationState::startedAtTick,
                0,
                kMostTick>("since")};

        constexpr std::array kHealthFields{
            wholeField<
                Health, &Health::food, 0, component::kFullHealth>(
                "food"),
            wholeField<
                Health, &Health::water, 0, component::kFullHealth>(
                "water")};

        constexpr std::array kInventoryFields{
            slotsField<Inventory, &Inventory::slots>("slots")};

        constexpr std::array kPatrolFields{
            wholeField<
                Patrol, &Patrol::nextStopIndex, 0, kMostIndex>("stop"),
            wholeField<
                Patrol, &Patrol::pathIndex, 0, kMostIndex>("path")};

        constexpr std::array kSpeakerFields{
            wholeField<
                Speaker, &Speaker::nextLineIndex, 0, kMostIndex>(
                "line")};

        constexpr std::array kCharacterIndexFields{
            wholeField<
                CharacterIndex, &CharacterIndex::index, 0, kMostIndex>(
                "index")};

        constexpr std::array kCarriedLightFields{
            tintField<CarriedLight, &CarriedLight::tintColor>("tint"),
            fixedField<
                CarriedLight,
                &CarriedLight::aboveHeight,
                0.0,
                kMostAbove>("above"),
            fixedField<
                CarriedLight,
                &CarriedLight::reach,
                0.0,
                kMostReach>("reach")};

        constexpr std::array kFillLightFields{
            tintField<FillLight, &FillLight::tintColor>("tint"),
            fixedField<
                FillLight,
                &FillLight::aboveHeight,
                0.0,
                kMostAbove>("above"),
            fixedField<
                FillLight, &FillLight::reach, 0.0, kMostReach>(
                "reach")};

        constexpr std::array kComponentRows{
            ComponentRow{
                .name = "component::Position",
                .role = Role::Derived,
                .fields = kPositionFields,
                .fresh = [] { return ComponentValue(Position{}); }},
            ComponentRow{
                .name = "component::Velocity",
                .role = Role::Valued,
                .fields = kVelocityFields,
                .fresh = [] { return ComponentValue(Velocity{}); }},
            ComponentRow{
                .name = "component::AnimationState",
                .role = Role::Derived,
                .fields = kAnimationStateFields,
                .fresh =
                    [] { return ComponentValue(AnimationState{}); }},
            ComponentRow{
                .name = "component::Health",
                .role = Role::Valued,
                .fields = kHealthFields,
                .fresh = [] { return ComponentValue(Health{}); }},
            ComponentRow{
                .name = "component::Inventory",
                .role = Role::Valued,
                .fields = kInventoryFields,
                .fresh = [] { return ComponentValue(Inventory{}); }},
            ComponentRow{
                .name = "component::Patrol",
                .role = Role::Valued,
                .fields = kPatrolFields,
                .fresh = [] { return ComponentValue(Patrol{}); }},
            ComponentRow{
                .name = "component::Speaker",
                .role = Role::Valued,
                .fields = kSpeakerFields,
                .fresh = [] { return ComponentValue(Speaker{}); }},
            ComponentRow{
                .name = "component::Player",
                .role = Role::Tag,
                .fields = {},
                .fresh = [] { return ComponentValue(Player{}); }},
            ComponentRow{
                .name = "component::CharacterIndex",
                .role = Role::Derived,
                .fields = kCharacterIndexFields,
                .fresh = [] { return ComponentValue(CharacterIndex{}); }},
            ComponentRow{
                .name = "component::CarriedLight",
                .role = Role::Valued,
                .fields = kCarriedLightFields,
                .fresh = [] { return ComponentValue(CarriedLight{}); }},
            ComponentRow{
                .name = "component::FillLight",
                .role = Role::Valued,
                .fields = kFillLightFields,
                .fresh = [] { return ComponentValue(FillLight{}); }}};
    }

    std::span<const ComponentRow> getComponentRows()
    {
        return kComponentRows;
    }

    const ComponentRow *getComponentRow(const std::string_view name)
    {
        const auto foundRow = std::ranges::find(
            kComponentRows, name, &ComponentRow::name);

        return foundRow == kComponentRows.end() ? nullptr : &*foundRow;
    }

}
