#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    /**
     * @brief Get which service a walker confers on what it passes.
     *
     * **The service half of carriedResource(), and deliberately a
     * separate table from it.** A good is an amount that changes hands
     * and a service is a state that is refreshed, so a walker does one
     * or the other and never both; the static_assert below is what says
     * so, and it is the only thing stopping a future kind from being
     * given a load and a service by two edits nobody read together.
     *
     * A cart pusher, a market buyer and a market seller are the errand
     * walkers, and an errand is a fact about where somebody is going
     * rather than about what walking past them does.
     *
     * @param kind The kind of walker.
     * @return The service its kind confers, or nullopt for one that
     * confers none.
     */
    [[nodiscard]] constexpr std::optional<Service> serviceConferredBy(
        WalkerKind kind) noexcept
    {
        constexpr std::array<
            std::optional<Service>, kWalkerKindCount> confers{
            Service::Water,       // WaterCarrier
            Service::Health,      // Doctor
            Service::Safety,      // Fireman
            Service::Structure,   // Engineer
            std::nullopt,         // CartPusher
            std::nullopt,         // MarketBuyer
            std::nullopt};        // MarketSeller

        return confers[walkerKindIndex(kind) % kWalkerKindCount];
    }

    // A walker hands an amount over or refreshes a state, never both.
    // Two tables are where that could quietly stop being true.
    static_assert(
        []
        {
            for (std::size_t index = 0; index < kWalkerKindCount; ++index)
            {
                const auto kind = static_cast<WalkerKind>(index);

                if (carriedResource(kind).has_value()
                    && serviceConferredBy(kind).has_value())
                {
                    return false;
                }
            }

            return true;
        }(),
        "a walker kind may carry a good or confer a service, not both");

    // Every service has somebody who confers it.
    // A service nothing can ever reach is a countdown that only falls.
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
        serviceConferredBy(WalkerKind::Fireman) == Service::Safety);
    static_assert(
        !serviceConferredBy(WalkerKind::CartPusher).has_value());

} // namespace antwika::game
