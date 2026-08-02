#pragma once

#include <cstdint>
#include <vector>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    /** @brief Where one mob stands and which kind it is. */
    struct MobMarker
    {
        Cell cell;
        MobKind kind = MobKind::Grunt;

        [[nodiscard]] bool operator==(const MobMarker &) const = default;
    };

    /**
     * @brief Everything a frame needs, and nothing that can change it.
     *
     * The spectator's answer to Campaign: cells and kinds rather than
     * mobs, so the scene never learns what a mob's health is or how a
     * tower picks a target.
     * The kind is in because it is what a mob is drawn as -- four
     * colours rather than one -- and it is the only thing about a mob
     * that is not either its position or a number the bar shows.
     * Rendering is a write-only projection here as everywhere else, and
     * a value the scene cannot write is the structural way to say so.
     */
    struct BattleSnapshot
    {
        /** @brief The grid being fought over. */
        const Level &level;

        /** @brief Where each living mob stands, and what it is. */
        std::vector<MobMarker> mobs;

        /** @brief Where each tower stands. */
        std::vector<Cell> towers;

        /** @brief Squared reach of every tower, in cells. */
        std::uint32_t towerRangeSquared = 0;
    };

    /**
     * @brief Take this tick's picture of the level being fought.
     * @param campaign The campaign to read; must outlive the snapshot,
     * which holds its level by reference.
     * @return The snapshot.
     */
    [[nodiscard]] BattleSnapshot snapshotOf(const Campaign &campaign);

    /**
     * @brief Get the whole-cell radius a squared range describes.
     * @param rangeSquared The squared reach, in cells.
     * @return The largest r with r * r <= rangeSquared, found by
     * counting rather than by a sqrt, so no float is involved.
     */
    [[nodiscard]] std::uint32_t rangeRadius(std::uint32_t rangeSquared);

} // namespace antwika::tower_defence
