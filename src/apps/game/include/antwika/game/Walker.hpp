#pragma once

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
     * Two, so a walker covers a cell every other tick rather than every
     * one.
     */
    inline constexpr std::uint8_t kTicksPerStep = 2;

    /**
     * @brief What a walker is carrying, and for whom.
     *
     * Declared beside Walker rather than beside WalkerView, because the
     * component is the truth and a view is a copy of it; a snapshot that
     * named its own kinds would be a second enumeration to keep in step.
     *
     * The order matches BuildingKind's sources, so which walker a
     * building sends is arithmetic rather than a switch.
     */
    enum class WalkerKind : std::uint8_t
    {
        Food = 0,
        Water,
        Fireman,
        Architect,
    };

    /**
     * @brief How many walker kinds there are.
     */
    inline constexpr std::size_t kWalkerKindCount =
        static_cast<std::size_t>(WalkerKind::Architect) + 1;

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
     * @brief Get which resource a walker hands out.
     * @param kind The kind of walker.
     * @return The resource it delivers, or nullopt for a fireman or an
     * architect, who carry nothing and relieve a risk instead.
     */
    [[nodiscard]] constexpr std::optional<Resource> carriedResource(
        WalkerKind kind) noexcept
    {
        if (kind == WalkerKind::Fireman || kind == WalkerKind::Architect)
        {
            return std::nullopt;
        }

        // The two carrying kinds list the two resources in one order.
        // So arithmetic rather than a switch, as turnRight() does.
        return static_cast<Resource>(walkerKindIndex(kind));
    }

    /**
     * @brief Get which walker a building sends out.
     * @param kind The building's kind; a house sends nobody.
     * @return The walker it sends, or nullopt for a house.
     */
    [[nodiscard]] constexpr std::optional<WalkerKind> walkerSentBy(
        BuildingKind kind) noexcept
    {
        if (!sendsWalkers(kind))
        {
            return std::nullopt;
        }

        // The sources are the kinds after House, in order.
        // And WalkerKind lists them in that same order.
        return static_cast<WalkerKind>(buildingKindIndex(kind) - 1);
    }

    static_assert(carriedResource(WalkerKind::Food) == Resource::Food);
    static_assert(carriedResource(WalkerKind::Water) == Resource::Water);
    static_assert(!carriedResource(WalkerKind::Fireman).has_value());
    static_assert(!walkerSentBy(BuildingKind::House).has_value());
    static_assert(walkerSentBy(BuildingKind::FoodSource) == WalkerKind::Food);
    static_assert(
        walkerSentBy(BuildingKind::ArchitectPost) == WalkerKind::Architect);
    static_assert(kWalkerKindCount == kBuildingKindCount - 1);

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

        /** @brief What it hands out, or what risk it relieves. */
        WalkerKind kind = WalkerKind::Food;

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
