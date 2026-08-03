#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief How many ticks one cell of walking takes.
     *
     * Eight, which is a quarter of the pace the walkers first shipped
     * at: a cell every other tick read as scurrying once the frames
     * between ticks slid a walker smoothly, and every period in the
     * economy -- coverage, drains, spawns -- was tuned generously
     * enough that the slower streets still keep a district served.
     */
    inline constexpr std::uint8_t kTicksPerStep = 8;

    /**
     * @brief What a walker is carrying, and for whom.
     *
     * Declared beside Walker rather than beside WalkerView, because the
     * component is the truth and a view is a copy of it; a snapshot that
     * named its own kinds would be a second enumeration to keep in step.
     *
     * **The order is this enumeration's own and matches nothing
     * else.** It used to run parallel to BuildingKind's sources so that
     * walkerSentBy() could be arithmetic; a market sends two kinds and a
     * storehouse sends none, so no offset can line the two up any more
     * and both crossings below are tables.
     */
    enum class WalkerKind : std::uint8_t
    {
        WaterCarrier = 0,  ///< Confers Service::Water.
        Doctor,            ///< Confers Service::Health.
        Fireman,           ///< Confers Service::Safety.
        Engineer,          ///< Confers Service::Structure.
        CartPusher,        ///< Hauls a load to a named store.
        MarketBuyer,       ///< Fetches an input from a store.
        MarketSeller,      ///< Hands goods out to houses.
        Migrant,           ///< Moving into a house, or leaving town.
        Labourer,          ///< Carries a house's workforce to jobs.
    };

    /**
     * @brief How many walker kinds there are.
     */
    inline constexpr std::size_t kWalkerKindCount =
        static_cast<std::size_t>(WalkerKind::Labourer) + 1;

    /**
     * @brief Get a kind's index, for addressing a per-kind table.
     * @param kind The kind to index.
     * @return The index, always below kWalkerKindCount.
     */
    [[nodiscard]] constexpr std::size_t walkerKindIndex(
        WalkerKind kind) noexcept
    {
        return static_cast<std::size_t>(kind);
    }

    /**
     * @brief How many cells a walker covers before it heads home.
     *
     * Counted in steps taken rather than in distance from where it
     * started, so a walker going round in circles tires exactly as fast
     * as one going in a straight line.
     */
    inline constexpr std::int32_t kRoamingSteps = 32;

    /**
     * @brief How much a walker that carries anything sets out with.
     */
    inline constexpr std::int32_t kWalkerLoad = 100;

    /**
     * @brief Get which resource a walker hands out by kind alone.
     *
     * **A table, and it answers about the kind rather than about the
     * walker.** A market seller sets out with food and a service walker
     * carries nothing, and both of those are facts about what somebody
     * *is*. What a cart pusher happens to be hauling is a fact about the
     * errand it is on rather than about its kind, so it answers nullopt
     * here and the errand says what is in the cart.
     *
     * @param kind The kind of walker.
     * @return The resource its kind always carries, or nullopt for one
     * that carries nothing, or nothing fixed.
     */
    [[nodiscard]] constexpr std::optional<Resource> carriedResource(
        WalkerKind kind) noexcept
    {
        constexpr std::array<
            std::optional<Resource>, kWalkerKindCount> carries{
            std::nullopt,     // WaterCarrier
            std::nullopt,     // Doctor
            std::nullopt,     // Fireman
            std::nullopt,     // Engineer
            std::nullopt,     // CartPusher
            std::nullopt,     // MarketBuyer
            Resource::Food,   // MarketSeller
            std::nullopt,     // Migrant
            std::nullopt};    // Labourer

        return carries[walkerKindIndex(kind) % kWalkerKindCount];
    }

    /**
     * @brief Get which walker a building sends out.
     *
     * **A table, for the reason buildingKindOf() is one.** The offset it
     * used to be was exact only while every kind but the house sent
     * exactly one walker, in one matching order; a storehouse sends
     * nobody from the middle of the enumeration and a market sends two
     * kinds, so no arithmetic lines the two up.
     *
     * A market answers with its seller, which is the walker its cadence
     * sends; the buyer it also sends is an errand rather than a cadence,
     * and belongs to whatever decides there is something to fetch.
     *
     * @param kind The building's kind.
     * @return The walker it sends, or nullopt for one that sends
     * nobody.
     */
    [[nodiscard]] constexpr std::optional<WalkerKind> walkerSentBy(
        BuildingKind kind) noexcept
    {
        constexpr std::array<
            std::optional<WalkerKind>, kBuildingKindCount> sends{
            std::nullopt,                // House
            WalkerKind::CartPusher,      // Farm
            WalkerKind::CartPusher,      // ClayPit
            WalkerKind::CartPusher,      // Workshop
            std::nullopt,                // Storage
            WalkerKind::MarketSeller,    // Market
            WalkerKind::WaterCarrier,    // Well
            WalkerKind::Doctor,          // Doctor
            WalkerKind::Fireman,         // FireStation
            WalkerKind::Engineer};       // EngineerPost

        return sends[buildingKindIndex(kind) % kBuildingKindCount];
    }

    // A kind that sends walkers has to name one.
    // And a kind that does not must name none.
    // Two tables saying one thing is where they can disagree.
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

    static_assert(
        carriedResource(WalkerKind::MarketSeller) == Resource::Food);
    static_assert(!carriedResource(WalkerKind::WaterCarrier).has_value());
    static_assert(!carriedResource(WalkerKind::CartPusher).has_value());
    static_assert(!walkerSentBy(BuildingKind::House).has_value());
    static_assert(
        walkerSentBy(BuildingKind::Market) == WalkerKind::MarketSeller);
    static_assert(
        walkerSentBy(BuildingKind::EngineerPost) == WalkerKind::Engineer);

    /**
     * @brief Something that walks the paths, and which way it is facing.
     *
     * Where it is lives in a separate Cell component, so a walker and a
     * path tile are told apart by which components they carry rather than
     * by a flag inside one shared type.
     */
    struct Walker
    {
        Direction facing = Direction::East;

        /** @brief What it hands out, or what service it confers. */
        WalkerKind kind = WalkerKind::WaterCarrier;

        /** @brief How much of its resource is left to hand out. */
        std::int32_t carried = 0;

        /**
         * @brief How many more cells it walks before it heads home.
         *
         * Held at zero once spent rather than going negative, which is
         * the same idiom Building's countdowns use.
         * Zero means heading home, and from then on the walker either
         * arrives or is destroyed -- see WalkerSystem.
         */
        std::int32_t stepsUntilHome = kRoamingSteps;

        /**
         * @brief The building that sent it out, if one did.
         *
         * kNullEntity for a walker nobody sent -- one dropped on a road
         * by hand, or restored from a save.
         * Such a walker roams its budget and is then destroyed, since
         * there is nowhere for it to path to; that is the same arm as a
         * walker whose building has since burned down, which is why the
         * two need no separate handling.
         */
        antwika::ecs::Entity home = antwika::ecs::kNullEntity;

        /**
         * @brief How many more ticks to wait before the next cell.
         *
         * The cadence is per walker and lives in the walker's own
         * component, rather than being a modulus on the tick number: two
         * walkers dropped a tick apart then keep their own rhythm, and
         * a replay regenerates each countdown from the same events that
         * created the walker.
         *
         * Zero on a fresh walker, so it sets off on the first tick it
         * sees.
         */
        std::uint8_t ticksUntilStep = 0;

        /**
         * @brief The cell this walker is stepping out of, if any.
         *
         * What a renderer needs to draw a walker part of the way between
         * two cells rather than jumping it a whole one.
         *
         * It is **simulation state and not a render-side channel**, which
         * is the distinction worth being careful about here: unlike
         * input::PointerHintChannel, a live run and its replay have to
         * agree on where a walker came from, because both of them draw
         * the same picture from it.
         *
         * Nothing but the previous cell will do. Working it back out as
         * step(at, opposite(facing)) is right in the middle of a
         * straight run and wrong exactly where there was no previous
         * cell at all -- a walker just placed, just spawned, restored
         * from a save, or sitting on a tile with no way off it. Those
         * are real states, and a plain Cell could only say so by naming
         * a cell the walker was never on.
         *
         * Absent on a fresh walker, so its first frame is drawn where it
         * stands rather than sliding in from somewhere it has never been.
         */
        std::optional<Cell> from{};

        /**
         * @brief Compare two walkers.
         * @param other The walker to compare against.
         * @return True when both face the same way, are the same far
         * through their step, and came from the same place.
         */
        [[nodiscard]] bool operator==(const Walker &other) const = default;
    };

} // namespace antwika::game
