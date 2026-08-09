#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/Resource.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    enum class ErrandLeg : std::uint8_t
    {
        Outbound = 0,
        Returning,
    };

    [[nodiscard]] constexpr ErrandLeg enumBound(ErrandLeg) noexcept
    {
        return ErrandLeg::Returning;
    }

    inline constexpr std::size_t kErrandLegCount =
        antwika::enums::kCount<ErrandLeg>;

    [[nodiscard]] constexpr std::size_t errandLegIndex(
        const ErrandLeg leg) noexcept
    {
        return antwika::enums::index(leg);
    }

    [[nodiscard]] constexpr std::string_view errandLegName(
        ErrandLeg leg) noexcept
    {
        constexpr std::array<std::string_view, kErrandLegCount> names{
            "outbound",
            "returning"};

        return antwika::enums::pick(names, leg);
    }

    [[nodiscard]] constexpr std::optional<ErrandLeg> errandLegFromName(
        std::string_view name) noexcept
    {
        for (std::size_t index = 0; index < kErrandLegCount; ++index)
        {
            const auto leg = static_cast<ErrandLeg>(index);

            if (errandLegName(leg) == name)
            {
                return leg;
            }
        }

        return std::nullopt;
    }

    static_assert(errandLegName(ErrandLeg::Outbound) == "outbound");
    static_assert(errandLegFromName("returning") == ErrandLeg::Returning);
    static_assert(!errandLegFromName("homeward").has_value());

    struct Errand final
    {
        antwika::ecs::Entity destination = antwika::ecs::kNullEntity;

        Resource carrying = Resource::Food;

        ErrandLeg leg = ErrandLeg::Outbound;

        [[nodiscard]] bool operator==(const Errand &other) const = default;
    };

    [[nodiscard]] constexpr antwika::ecs::Entity errandTarget(
        const Errand &errand, const Walker &walker) noexcept
    {
        return errand.leg == ErrandLeg::Outbound ? errand.destination
                                                 : walker.home;
    }

    [[nodiscard]] antwika::ecs::Entity errandTargetOf(
        const antwika::ecs::World &world, antwika::ecs::Entity entity);

}
