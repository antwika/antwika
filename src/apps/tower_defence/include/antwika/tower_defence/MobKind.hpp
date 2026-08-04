#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::tower_defence
{

    /**
     * @brief The kinds of walker a wave can be made of.
     *
     * Four rather than one with a different colour, and each differs on
     * numbers that pull against each other rather than on one dial.
     * The numbering is the order kAllMobKinds lists them in and the
     * order a wave's entries are expanded in, so it is a total order a
     * spawn sequence can be built from.
     */
    enum class MobKind : std::uint8_t
    {
        /** @brief The ordinary walker every level opens with. */
        Grunt = 0,

        /** @brief Half the time in a tower's reach, and less health. */
        Runner = 1,

        /** @brief Three times the time in reach, and a lot of health. */
        Brute = 2,

        /** @brief Takes armour off every hit, so a weak gun is wasted. */
        Shielded = 3,
    };

    /** @brief How many kinds there are. */
    inline constexpr std::size_t kMobKindCount = 4;

    /** @brief Every kind, in the enumeration's own order. */
    inline constexpr std::array<MobKind, kMobKindCount> kAllMobKinds{
        MobKind::Grunt,
        MobKind::Runner,
        MobKind::Brute,
        MobKind::Shielded};

    /**
     * @brief Everything one kind of walker is worth and costs.
     *
     * Integers throughout, like the rest of the simulation, so a replay
     * that regenerates the level regenerates every hit point of it.
     */
    struct MobProfile
    {
        /**
         * @brief How many ticks it takes to cross one path cell.
         *
         * At least one, so a mob always moves eventually.
         * This is the axis that interacts with where a tower goes: the
         * damage a mob takes crossing a tower's reach is the cells in
         * reach times this, so a Brute at three pays a corner tower
         * three times what a Runner at one does.
         */
        std::uint32_t ticksPerCell = 1;

        /** @brief Health it spawns with. */
        std::int32_t health = 1;

        /**
         * @brief Damage taken off every hit before it lands.
         *
         * A tower whose damage does not exceed this cannot hurt the mob
         * at all, and no number of them can either, since each tower
         * fires on its own.
         * That is a level's decision rather than an accident:
         * campaignLevels() only puts an armoured kind in a level whose
         * towers out-damage it, and CampaignTest asserts that of the
         * shipped table.
         */
        std::int32_t armour = 0;

        /** @brief Score for killing one. */
        std::uint64_t reward = 0;

        /**
         * @brief Compare two profiles.
         * @param other The profile to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const MobProfile &other) const = default;
    };

    /**
     * @brief Get the numbers one kind is balanced with.
     * @param kind The kind to describe; one of the enumerators.
     * @return Its profile.
     */
    /**
     * @brief What each kind is worth, costs and survives, in MobKind
     * order.
     *
     * The four pull against each other rather than ranking: a Runner
     * is through a reach in a third of a Brute's ticks, a Brute is
     * worth more but blocks nothing while it plods, and a Shielded one
     * is only worth building for once guns hit hard.
     *
     * A lookup a config file can restate rather than a table only this
     * build can see; profileOf() answers from this, and BattleConfig
     * carries whichever table a run was configured with.
     */
    inline constexpr std::array<MobProfile, kMobKindCount>
        kDefaultMobProfiles{
            MobProfile{
                .ticksPerCell = 2, .health = 6, .armour = 0, .reward = 10},
            MobProfile{
                .ticksPerCell = 1, .health = 4, .armour = 0, .reward = 14},
            MobProfile{
                .ticksPerCell = 3,
                .health = 18,
                .armour = 0,
                .reward = 24},
            MobProfile{
                .ticksPerCell = 2,
                .health = 8,
                .armour = 1,
                .reward = 30}};

    [[nodiscard]] MobProfile profileOf(MobKind kind);

} // namespace antwika::tower_defence
