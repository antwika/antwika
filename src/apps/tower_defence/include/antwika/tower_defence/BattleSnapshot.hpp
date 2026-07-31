#pragma once

#include <cstdint>
#include <vector>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/Level.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief Everything a frame needs, and nothing that can change it.
     *
     * The spectator's answer to Battle: cells rather than mobs, so the
     * scene never learns what a mob's health is or how a tower picks a
     * target.
     * Rendering is a write-only projection here as everywhere else, and
     * a value the scene cannot write is the structural way to say so.
     */
    struct BattleSnapshot
    {
        /** @brief The grid being fought over. */
        const Level &level;

        /** @brief Where each living mob currently stands. */
        std::vector<Cell> mobs;

        /** @brief Where each tower stands. */
        std::vector<Cell> towers;

        /** @brief Squared reach of every tower, in cells. */
        std::uint32_t towerRangeSquared = 0;

        std::uint64_t score = 0;
        std::uint32_t leaks = 0;
    };

    /**
     * @brief Take this tick's picture of a battle.
     * @param battle The battle to read; must outlive the snapshot, which
     * holds its level by reference.
     * @return The snapshot.
     */
    [[nodiscard]] BattleSnapshot snapshotOf(const Battle &battle);

    /**
     * @brief Get the whole-cell radius a squared range describes.
     * @param rangeSquared The squared reach, in cells.
     * @return The largest r with r * r <= rangeSquared, found by
     * counting rather than by a sqrt, so no float is involved.
     */
    [[nodiscard]] std::uint32_t rangeRadius(std::uint32_t rangeSquared);

} // namespace antwika::tower_defence
