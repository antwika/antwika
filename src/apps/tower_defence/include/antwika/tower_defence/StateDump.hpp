#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/tower_defence/Campaign.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief Thrown when a state-dump document is not one this build
     * can read.
     *
     * That covers a document failing the schema, a mob kind or a
     * campaign phase named by a name this build does not know, and a
     * missing or misshapen member.
     *
     * Its own type rather than ScoreFormatError, following the
     * one-exception-type-per-failure-category rule: a dump and a high
     * score are different documents with different readers.
     * TowerDefenceSnapshotStore rewraps it into console::SnapshotError,
     * which is what the console seam promises.
     */
    class StateDumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief What every document of this format says it is.
     *
     * The magic names the application, so another app's dump is
     * refused before any state is looked at -- see
     * console::SnapshotFormat.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-tower-defence-state-dump";

    /**
     * @brief Which revision of the dump format this build writes.
     *
     * Version 1 is the first, so standardStateDumpMigrations() hands
     * back an empty chain; see HighScore.hpp for the whole argument.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief This application's whole state, as dump_state takes it.
     *
     * The campaign carries the battle inside it, and the level and the
     * wave plan are regenerated from the seed on restore rather than
     * written down -- see CampaignMemory.
     */
    struct StateDump
    {
        /** @brief Everything the run has moved. */
        CampaignMemory campaign;

        /**
         * @brief The best-score baseline the run's bar shows.
         *
         * Run configuration rather than simulation state -- it was
         * read off the high-score file once, before the loop -- but it
         * is carried so a restored bar shows the record the dumped run
         * was playing against.
         */
        std::uint64_t bestScore = 0;

        [[nodiscard]] bool operator==(const StateDump &) const
            = default;
    };

    /**
     * @brief Build the migration chain for the dump format.
     * @return The chain, currently with no steps in it.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode a dump as the state object the envelope carries.
     * @param dump What to write.
     * @return The state object, without the envelope.
     */
    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    /**
     * @brief Decode a dump from the envelope's state object,
     * validating it first.
     *
     * A mob's pathIndex is deliberately not ranged here: the path it
     * indexes exists only once the level is regenerated, so
     * Campaign::restore() is what refuses one standing past it.
     *
     * @param state The state object the envelope carried.
     * @return What it holds.
     * @throws StateDumpError If the object fails the schema, or names
     * a mob kind or a campaign phase this build does not know.
     */
    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

} // namespace antwika::tower_defence
