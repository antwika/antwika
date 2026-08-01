#pragma once

#include <optional>

#include "antwika/tower_defence/HighScore.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief Where the best a campaign has been played to is kept
     * between one run and the next.
     *
     * The one seam between this application and a filesystem, in
     * companion::IPetStore's shape and for its reason: every other class
     * here is exercised with no file on disk at all, because a test
     * hands the run a store that answers from memory and the run cannot
     * tell the difference.
     */
    class IScoreStore
    {
    public:
        virtual ~IScoreStore() = default;

        /**
         * @brief Read the best any earlier run reached.
         * @return What it was, or nothing when there is no record yet --
         * a first run, or a store given nowhere to look. That is an
         * ordinary answer worth a best of zero, and never stops a game.
         * @throws ScoreFormatError If there is something to read and it
         * is not a record this build can read.
         */
        [[nodiscard]] virtual std::optional<HighScore> load() = 0;

        /**
         * @brief Write the record out.
         * @param score The record to keep.
         * @throws ScoreFormatError If the bytes cannot be written.
         */
        virtual void save(const HighScore &score) = 0;
    };

} // namespace antwika::tower_defence
