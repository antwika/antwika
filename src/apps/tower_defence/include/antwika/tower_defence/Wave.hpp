#pragma once

#include <cstdint>
#include <vector>

#include <antwika/rng/IRng.hpp>

#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    using antwika::rng::IRng;

    /** @brief So many of one kind, somewhere in a wave. */
    struct WaveEntry
    {
        MobKind kind = MobKind::Grunt;
        std::uint32_t count = 0;

        [[nodiscard]] bool operator==(const WaveEntry &) const = default;
    };

    /**
     * @brief One wave, as a level's designer states it.
     *
     * A composition rather than a sequence: the entries say what is in
     * the wave and planWaves() decides what order it comes out in, so a
     * level can ask for "four grunts and two runners" without also
     * having to decide which of the six is first.
     */
    struct Wave
    {
        /** @brief What the wave is made of. */
        std::vector<WaveEntry> entries;

        /** @brief Ticks between one mob of this wave and the next. */
        std::uint64_t spawnPeriodTicks = 4;

        /**
         * @brief Ticks of quiet after the last of this wave is released.
         *
         * The gap is what makes a wave a wave rather than a stream: it
         * is the room a player has to build before the next one starts.
         * The last wave of a level may as well ask for none, since
         * finishing it finishes the level.
         */
        std::uint64_t gapTicks = 24;
    };

    /**
     * @brief One wave with its order already decided.
     *
     * What Battle is handed, so nothing inside the tick path ever draws
     * a random number: every draw happens once, before the level starts,
     * from a generator seeded off the campaign's seed.
     */
    struct WaveRelease
    {
        /** @brief The kinds to release, in the order they are spawned. */
        std::vector<MobKind> order;

        std::uint64_t spawnPeriodTicks = 4;
        std::uint64_t gapTicks = 0;
    };

    /**
     * @brief Expand a wave's composition into a spawn order.
     *
     * The entries are expanded in the order they are written -- kind by
     * kind, count by count -- and the resulting bag is then shuffled by
     * a Fisher-Yates pass driven by the injected generator.
     * Shuffling rather than releasing kind after kind is what stops a
     * mixed wave from being three separate ones in a row, and drawing
     * the shuffle from a seed is what keeps it reproducible: the same
     * seed gives the same order on every toolchain, since the swap index
     * is a modulo of raw bits and not a std::uniform_int_distribution.
     *
     * @param waves The level's waves, in the order they are fought.
     * @param rng Draws the shuffle; advanced once per swap.
     * @return One release per wave, in the same order.
     */
    [[nodiscard]] std::vector<WaveRelease> planWaves(
        const std::vector<Wave> &waves, IRng &rng);

    /**
     * @brief How many mobs a wave releases in total.
     * @param wave The wave to measure.
     * @return The sum of its entries' counts.
     */
    [[nodiscard]] std::uint32_t waveSize(const Wave &wave);

} // namespace antwika::tower_defence
