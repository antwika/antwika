#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    /**
     * @brief How well a household lives, and therefore how many of them
     * live there.
     *
     * Four tiers, which is the fewest that shows the loop the genre is
     * about: a bottom one nothing is demanded of, a top one, and two in
     * between so "evolves" and "devolves" are both ordinary rather than
     * both being the edge of the ladder.
     *
     * Values are contiguous from zero, so a level can index a table and
     * the next tier up is one more -- which is what lets the whole rule
     * be "does it meet the requirements of level + 1" rather than a
     * switch that grows an arm per tier.
     */
    enum class HousingLevel : std::uint8_t
    {
        Tent = 0,  ///< What a house is the moment it is put up.
        Shack,     ///< Watered.
        Hovel,     ///< Watered, fed and somewhere worth living.
        Cottage,   ///< All of that, and a doctor.
    };

    /**
     * @brief How many housing levels there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kHousingLevelCount =
        static_cast<std::size_t>(HousingLevel::Cottage) + 1;

    /**
     * @brief Get a level's index, for addressing a per-level table.
     * @param level The level to index.
     * @return The index, always below kHousingLevelCount.
     */
    [[nodiscard]] constexpr std::size_t housingLevelIndex(
        HousingLevel level) noexcept
    {
        return static_cast<std::size_t>(level);
    }

    /**
     * @brief Every level, in the enumeration's own order.
     *
     * What anything wanting one answer per level iterates, so a fifth
     * tier is an enumerator here and a row in the table below and
     * nowhere else.
     */
    inline constexpr std::array<HousingLevel, kHousingLevelCount>
        kHousingLevels{
            HousingLevel::Tent,
            HousingLevel::Shack,
            HousingLevel::Hovel,
            HousingLevel::Cottage};

    /**
     * @brief Get a level's name.
     *
     * Symbolic rather than the enumerator's number, for the reason a
     * direction is written by name in a save: a name survives the
     * enumeration being reordered, and being hand-editable is most of why
     * that format is JSON.
     * These are the names a file writes, not the captions a reader sees
     * -- ReadoutPanel keeps a table of its own for those, exactly as it
     * does for a building kind.
     *
     * @param level The level to name.
     * @return Its name, in the enumeration's own order.
     */
    [[nodiscard]] constexpr std::string_view housingLevelName(
        HousingLevel level) noexcept
    {
        constexpr std::array<std::string_view, kHousingLevelCount> names{
            "tent",
            "shack",
            "hovel",
            "cottage"};

        return names[housingLevelIndex(level) % kHousingLevelCount];
    }

    /**
     * @brief Get the level a name refers to.
     * @param name The name to look up.
     * @return The level, or nullopt when no level has that name.
     */
    [[nodiscard]] constexpr std::optional<HousingLevel>
        housingLevelFromName(std::string_view name) noexcept
    {
        for (std::size_t index = 0; index < kHousingLevelCount; ++index)
        {
            const auto level = static_cast<HousingLevel>(index);

            if (housingLevelName(level) == name)
            {
                return level;
            }
        }

        return std::nullopt;
    }

    /**
     * @brief Check whether people live in this kind of building.
     *
     * A table rather than a comparison against House, for the reason
     * consumes() is one: the second kind somebody lives in -- a block of
     * flats, a villa -- would otherwise be a second name in the same
     * expression, and an offset past a hole still lands on a valid
     * enumerator.
     *
     * @param kind The kind to ask about.
     * @return True for a kind that has a household in it.
     */
    [[nodiscard]] constexpr bool housesPeople(BuildingKind kind) noexcept
    {
        constexpr std::array<bool, kBuildingKindCount> housing{
            true,   // House
            false,  // Farm
            false,  // ClayPit
            false,  // Workshop
            false,  // Storage
            false,  // Market
            false,  // Well
            false,  // Doctor
            false,  // FireStation
            false,  // EngineerPost
        };

        return housing[buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief What a house must have to hold one level, and what it gets
     * for it.
     *
     * Integers and flags throughout, like everything else a replay has to
     * reproduce; there is not a fraction or a rate anywhere in it.
     */
    struct HousingRequirement
    {
        /**
         * @brief How pleasant the cell it stands on has to be.
         *
         * Read off the DesirabilityField at the house's own origin cell,
         * so a block that reached across two values is judged at one of
         * them rather than at an average nobody could point at.
         */
        std::int32_t desirability = 0;

        /**
         * @brief Which services still have to be reaching it.
         *
         * A flag per Service rather than a list, indexed by
         * serviceIndex(), so a fifth service is a column here and no
         * change to the code that reads it.
         * Only that a service reaches at all is asked, never how much of
         * it is left: coverage is a countdown whose whole job is to say
         * whether somebody came recently, and a threshold on it would be
         * a second, unstated one.
         */
        std::array<bool, kServiceCount> services{};

        /**
         * @brief How much of each good it has to be holding.
         *
         * Indexed by resourceIndex(), exactly as Building::stock is.
         * Zero means the good is not asked for at all.
         */
        std::array<std::int32_t, kResourceCount> goods{};

        /**
         * @brief How many people the level houses.
         *
         * **Stored here in W3 and moved by nobody in W3.** A level's
         * capacity and a house's occupancy are one fact, so the number
         * lives beside the level; the rules that raise and lower an
         * occupancy are a later workstream's, and this is the ceiling
         * they will read.
         */
        std::int32_t populationCapacity = 0;

        /**
         * @brief Compare two requirements.
         * @param other The requirement to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] constexpr bool operator==(
            const HousingRequirement &other) const = default;
    };

    /**
     * @brief What each level demands, and what it houses.
     *
     * **The bottom row demands nothing, and that is load-bearing rather
     * than decorative.** A house always meets its own bottom level, so
     * there is no rule saying it cannot devolve below one -- it simply
     * never fails the row it is standing on.
     *
     * **Only food is ever demanded, and the static_assert below is what
     * says why.** A market seller carries exactly one resource, so food
     * is the only good that ever reaches a house at all; clay and
     * pottery both move between a storehouse and a workshop and neither
     * is ever handed to anybody who lives anywhere. A tier that
     * demanded either would be a tier no city could reach, which is a
     * worse bug than a short ladder because nothing would say so.
     *
     * The desirability figures are read off kDesirabilityOf and its
     * linear falloff rather than guessed: a well is worth nothing to its
     * neighbours once the integer division truncates, so 1 means a market
     * or a doctor within two cells and 2 means both.
     */
    inline constexpr std::array<HousingRequirement, kHousingLevelCount>
        kHousingRequirements{{
            // Tent: what a house is the moment it is put up.
            {.desirability = 0,
             .services = {},
             .goods = {},
             .populationCapacity = 5},

            // Shack: somebody brings the water.
            {.desirability = 0,
             .services = {true, false, false, false},
             .goods = {},
             .populationCapacity = 10},

            // Hovel: and the food, somewhere worth living.
            {.desirability = 1,
             .services = {true, false, false, false},
             .goods = {25, 0, 0},
             .populationCapacity = 16},

            // Cottage: and a doctor.
            {.desirability = 2,
             .services = {true, true, false, false},
             .goods = {50, 0, 0},
             .populationCapacity = 24},
        }};

    /**
     * @brief Get what one level demands and houses.
     * @param level The level to ask about.
     * @return Its row of the table.
     */
    [[nodiscard]] constexpr HousingRequirement requirementOf(
        HousingLevel level) noexcept
    {
        return kHousingRequirements[
            housingLevelIndex(level) % kHousingLevelCount];
    }

    /**
     * @brief Ticks a house must go on qualifying before it grows.
     *
     * Four seconds, which is one drain period: long enough that a single
     * seller passing by does not promote a house on his own, and short
     * enough that a district put together properly shows for it within a
     * run somebody would sit through.
     */
    inline constexpr std::int32_t kEvolvePeriodTicks =
        4 * kTicksPerSecond;

    /**
     * @brief Ticks a house must go on falling short before it shrinks.
     *
     * The same span the other way. They are two constants rather than one
     * because they are two decisions -- a city ought to be able to punish
     * neglect faster than it rewards care, or the reverse -- and this
     * increment has no evidence for either, so it makes them equal and
     * says so here rather than pretending there is only one number.
     */
    inline constexpr std::int32_t kDevolvePeriodTicks =
        4 * kTicksPerSecond;

    // Indexing kHousingLevels by a level must hand that level back.
    // Otherwise a table walked in this order is a table read wrongly.
    // Resource.hpp and Service.hpp state the same rule about their lists.
    static_assert(
        []
        {
            for (std::size_t index = 0; index < kHousingLevelCount; ++index)
            {
                if (housingLevelIndex(kHousingLevels[index]) != index)
                {
                    return false;
                }
            }

            return true;
        }(),
        "kHousingLevels must list every level in its own index order");

    // The bottom level must demand nothing at all.
    // It is what a house standing on bare ground already meets.
    // So "never devolves below the bottom" needs no rule of its own.
    static_assert(
        kHousingRequirements[0]
            == HousingRequirement{
                .desirability = 0,
                .services = {},
                .goods = {},
                .populationCapacity =
                    kHousingRequirements[0].populationCapacity},
        "the bottom level must demand nothing");

    // Every demand must rise with the level, and this is why.
    // Meeting the next level's row then implies meeting this one's.
    // So a house can never be owed an evolve and a devolve at once.
    // HousingSystem depends on that rather than checking for it.
    static_assert(
        []
        {
            for (std::size_t index = 1; index < kHousingLevelCount; ++index)
            {
                const auto &below = kHousingRequirements[index - 1];
                const auto &here = kHousingRequirements[index];

                if (here.desirability < below.desirability
                    || here.populationCapacity < below.populationCapacity)
                {
                    return false;
                }

                for (std::size_t slot = 0; slot < kServiceCount; ++slot)
                {
                    if (below.services[slot] && !here.services[slot])
                    {
                        return false;
                    }
                }

                for (std::size_t slot = 0; slot < kResourceCount; ++slot)
                {
                    if (here.goods[slot] < below.goods[slot])
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "every housing demand must rise with the level");

    // A level may only demand a good a house can actually be given.
    // A market seller is the one walker that hands goods to a house.
    // And carriedResource() says it sets out with exactly one of them.
    // So a tier naming any other good is a tier no city could reach.
    // Which is worse than a short ladder, because nothing would say so.
    static_assert(
        []
        {
            for (const auto &requirement : kHousingRequirements)
            {
                for (const auto resource : kResources)
                {
                    if (requirement.goods[resourceIndex(resource)] > 0
                        && carriedResource(WalkerKind::MarketSeller)
                            != resource)
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a level may only demand a good a market seller carries");

    // And only as much of it as a house can hold at once.
    // A demand above capacity is a tier nothing ever reaches either.
    // Store.hpp already pins a house's capacity to this constant.
    static_assert(
        []
        {
            for (const auto &requirement : kHousingRequirements)
            {
                for (const auto resource : kResources)
                {
                    if (requirement.goods[resourceIndex(resource)]
                        > kStockCapacity)
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a level may only demand what a house can hold");

    static_assert(housingLevelName(HousingLevel::Tent) == "tent");
    static_assert(
        housingLevelFromName("cottage") == HousingLevel::Cottage);
    static_assert(!housingLevelFromName("villa").has_value());
    static_assert(housesPeople(BuildingKind::House));
    static_assert(!housesPeople(BuildingKind::Market));

} // namespace antwika::game
