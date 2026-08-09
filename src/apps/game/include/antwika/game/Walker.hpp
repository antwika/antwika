#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/ecs/Entity.hpp>
#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"

namespace antwika::game
{

    inline constexpr std::uint8_t kTicksPerStep = 8;

    enum class WalkerKind : std::uint8_t
    {
        WaterCarrier = 0,
        Doctor,
        Fireman,
        Engineer,
        CartPusher,
        MarketBuyer,
        MarketSeller,
        Migrant,
        Labourer,
    };

    [[nodiscard]] constexpr WalkerKind enumBound(WalkerKind) noexcept
    {
        return WalkerKind::Labourer;
    }

    inline constexpr std::size_t kWalkerKindCount =
        antwika::enums::kCount<WalkerKind>;

    [[nodiscard]] constexpr std::size_t walkerKindIndex(
        const WalkerKind kind) noexcept
    {
        return antwika::enums::index(kind);
    }

    inline constexpr std::int32_t kRoamingSteps = 32;

    inline constexpr std::int32_t kWalkerLoad = 100;

    [[nodiscard]] constexpr bool carriesGoods(WalkerKind kind) noexcept
    {
        constexpr std::array<bool, kWalkerKindCount> carries{
            false,
            false,
            false,
            false,
            true,
            true,
            true,
            false,
            false};

        return antwika::enums::pick(carries, kind);
    }

    [[nodiscard]] constexpr std::optional<WalkerKind> walkerSentBy(
        BuildingKind kind) noexcept
    {
        constexpr std::array<
            std::optional<WalkerKind>, kBuildingKindCount> sends{
            std::nullopt,
            WalkerKind::CartPusher,
            WalkerKind::CartPusher,
            WalkerKind::CartPusher,
            std::nullopt,
            WalkerKind::MarketSeller,
            WalkerKind::WaterCarrier,
            WalkerKind::Doctor,
            WalkerKind::Fireman,
            WalkerKind::Engineer};

        return antwika::enums::pick(sends, kind);
    }

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount; ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);

                if (sendsWalkers(kind) != walkerSentBy(kind).has_value())
                {
                    return false;
                }
            }

            return true;
        }(),
        "sendsWalkers() and walkerSentBy() must agree on every kind");

    static_assert(carriesGoods(WalkerKind::MarketSeller));
    static_assert(carriesGoods(WalkerKind::CartPusher));
    static_assert(!carriesGoods(WalkerKind::WaterCarrier));
    static_assert(!walkerSentBy(BuildingKind::House).has_value());
    static_assert(
        walkerSentBy(BuildingKind::Market) == WalkerKind::MarketSeller);
    static_assert(
        walkerSentBy(BuildingKind::EngineerPost) == WalkerKind::Engineer);

    struct Walker final
    {
        Direction facing = Direction::East;

        WalkerKind kind = WalkerKind::WaterCarrier;

        std::int32_t carried = 0;

        std::int32_t stepsUntilHome = kRoamingSteps;

        antwika::ecs::Entity home = antwika::ecs::kNullEntity;

        std::uint8_t ticksUntilStep = 0;

        std::optional<Cell> from{};

        [[nodiscard]] bool operator==(const Walker &other) const = default;
    };

}
