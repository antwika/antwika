#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Resource.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    /**
     * @brief Which half of a round trip a walker is on.
     *
     * Values are contiguous from zero, so a leg can index a table.
     */
    enum class ErrandLeg : std::uint8_t
    {
        Outbound = 0,  ///< Heading for the errand's destination.
        Returning,     ///< Heading back to the building that sent it.
    };

    /**
     * @brief How many legs there are.
     */
    inline constexpr std::size_t kErrandLegCount =
        static_cast<std::size_t>(ErrandLeg::Returning) + 1;

    /**
     * @brief Get a leg's index, for addressing a per-leg table.
     * @param leg The leg to index.
     * @return The index, always below kErrandLegCount.
     */
    [[nodiscard]] constexpr std::size_t errandLegIndex(
        ErrandLeg leg) noexcept
    {
        return static_cast<std::size_t>(leg);
    }

    /**
     * @brief Get a leg's name.
     *
     * Symbolic rather than the enumerator's number, for the reason a
     * direction is written by name in a save: a name survives the
     * enumeration being reordered, and being hand-editable is most of
     * why that format is JSON.
     *
     * @param leg The leg to name.
     * @return Its name, in the enumeration's own order.
     */
    [[nodiscard]] constexpr std::string_view errandLegName(
        ErrandLeg leg) noexcept
    {
        constexpr std::array<std::string_view, kErrandLegCount> names{
            "outbound",
            "returning"};

        return names[errandLegIndex(leg) % kErrandLegCount];
    }

    /**
     * @brief Get the leg a name refers to.
     * @param name The name to look up.
     * @return The leg, or nullopt when no leg has that name.
     */
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

    /**
     * @brief Where a walker is taking a load, and what is in it.
     *
     * **A walker with one of these is routed; a walker without one
     * roams**, which is what keeps WalkerSystem unchanged for every
     * walker that existed before this component did.
     *
     * The destination may be kNullEntity, and that is an ordinary state
     * rather than a missing one: a cart pusher loaded in a city with
     * nowhere to unload takes the load along its rounds instead, handing
     * it to whatever it walks past, exactly as the food walker of the
     * version-2 vocabulary did. That is what keeps a city migrated from
     * such a file fed while it has no storehouse yet, and it is the
     * reason this is one component rather than a routed one and a
     * loaded one.
     *
     * What is in the cart lives here rather than being read off the
     * walker's kind, because carriedResource() answers about a *kind* --
     * a market seller always sets out with food -- and what a cart
     * pusher happens to be hauling is a fact about the errand.
     */
    struct Errand
    {
        /**
         * @brief The building the load is bound for.
         *
         * kNullEntity for a load with nowhere to go; see the class
         * comment.
         * A handle rather than a cell, so a destination demolished
         * mid-route is answered by the world rather than by a stale
         * coordinate that now names bare ground.
         */
        antwika::ecs::Entity destination = antwika::ecs::kNullEntity;

        /** @brief What is in the cart. */
        Resource carrying = Resource::Food;

        /** @brief Which half of the round trip it is on. */
        ErrandLeg leg = ErrandLeg::Outbound;

        /**
         * @brief Compare two errands.
         * @param other The errand to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Errand &other) const = default;
    };

    /**
     * @brief Get the building an errand is heading for right now.
     *
     * The destination on the way out and the building that sent it on
     * the way back, which is the whole of what a leg means.
     *
     * @param errand The errand to read.
     * @param walker The walker carrying it, for where it came from.
     * @return The building it is bound for, or kNullEntity when it is
     * bound for nowhere.
     */
    [[nodiscard]] constexpr antwika::ecs::Entity errandTarget(
        const Errand &errand, const Walker &walker) noexcept
    {
        return errand.leg == ErrandLeg::Outbound ? errand.destination
                                                 : walker.home;
    }

    /**
     * @brief Get the building an entity's errand is heading for.
     *
     * The one question WalkerSystem and BuildingSystem both ask, so it
     * is answered in one place: kNullEntity covers a walker with no
     * errand, a walker whose errand names nowhere, and a walker heading
     * back to a building that never existed, and all three mean "not
     * routed" to both callers.
     *
     * @param world The world to read, as of its last commit().
     * @param entity The walker to ask about.
     * @return The building it is bound for, or kNullEntity.
     */
    [[nodiscard]] antwika::ecs::Entity errandTargetOf(
        const antwika::ecs::World &world, antwika::ecs::Entity entity);

} // namespace antwika::game
