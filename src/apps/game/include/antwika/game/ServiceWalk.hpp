#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    [[nodiscard]] constexpr std::optional<Service> serviceConferredBy(
        WalkerKind kind) noexcept
    {
        constexpr std::array<
            std::optional<Service>, kWalkerKindCount> confers{
            Service::Water,
            Service::Health,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt};

        return antwika::enums::pick(confers, kind);
    }

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kWalkerKindCount; ++index)
            {
                const auto kind = static_cast<WalkerKind>(index);

                if (carriesGoods(kind)
                    && serviceConferredBy(kind).has_value())
                {
                    return false;
                }
            }

            return true;
        }(),
        "a walker kind may carry a good or confer a service, not both");

    static_assert(
        []
        {
            for (const auto service : kServices)
            {
                bool conferred = false;

                for (std::size_t index = 0; index < kWalkerKindCount;
                     ++index)
                {
                    const auto kind = static_cast<WalkerKind>(index);

                    conferred = conferred
                        || serviceConferredBy(kind) == service;
                }

                if (!conferred)
                {
                    return false;
                }
            }

            return true;
        }(),
        "every service must have a walker kind that confers it");

    static_assert(
        serviceConferredBy(WalkerKind::Doctor) == Service::Health);
    static_assert(
        !serviceConferredBy(WalkerKind::Fireman).has_value());
    static_assert(
        !serviceConferredBy(WalkerKind::CartPusher).has_value());

}
