#pragma once

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

namespace antwika::tower_defence
{

    using antwika::replay::MigrationChain;

    /**
     * @brief The best a campaign has ever been played to.
     *
     * Two numbers rather than one: the score alone cannot tell a run
     * that scraped through three levels from one that farmed the first,
     * and how far somebody got is the other half of "best".
     */
    struct HighScore
    {
        /** @brief The highest score any run has finished on. */
        std::uint64_t bestScore = 0;

        /**
         * @brief The furthest level any run reached, counting from one.
         *
         * Zero for a first run, which has reached no level at all.
         */
        std::size_t bestLevel = 0;

        [[nodiscard]] bool operator==(const HighScore &) const = default;
    };

    /**
     * @brief What every document of this format says it is.
     *
     * Checked before anything else is read, so a replay or a companion
     * handed to this loader is refused as the wrong kind of file rather
     * than as a high score with every member missing -- which matters
     * because all of them state their version in the same member, and
     * the magic is the only thing telling them apart.
     */
    inline constexpr std::string_view kScoreMagic =
        "antwika-tower-defence-score";

    /**
     * @brief Which revision of the high-score format this build writes.
     *
     * Stated in antwika::replay::kSchemaVersionKey -- "version" -- the
     * one member every persisted document in this code base carries its
     * version in.
     *
     * Version 1 is the first, so standardScoreMigrations() hands back an
     * empty chain: a format with one revision has no step to take, and
     * writing a migration for a version that never shipped would be
     * fiction. The chain is still what reads every document, so the
     * first bump is one entry in that list and nothing else -- and until
     * then it is what refuses a file from a build newer than this one.
     *
     * A bump is needed when a document written under N is no longer
     * valid, or is still valid and means something else. Adding an
     * optional member is not that. See docs/schema-versioning.md.
     */
    inline constexpr std::uint32_t kScoreFormatVersion = 1;

    /**
     * @brief Build the migration chain for the high-score format.
     *
     * This format's answer to standardReplayMigrations(), and the whole
     * reason MigrationChain is generic over the document: it names its
     * own list, its own current version and nothing else.
     * There is no registry, so this chain and the replay's cannot see
     * each other.
     *
     * @return The chain, currently with no steps in it.
     */
    [[nodiscard]] MigrationChain standardScoreMigrations();

    /**
     * @brief Encode a high score as one JSON document.
     * @param score What to write.
     * @return The document.
     */
    [[nodiscard]] nlohmann::json highScoreToJson(const HighScore &score);

    /**
     * @brief Decode a high score from one JSON document.
     *
     * Reads it as `parse -> read version -> migrate -> validate ->
     * decode`, migrating before validating so that exactly one schema
     * exists rather than one per revision.
     *
     * @param document The parsed document.
     * @return What it holds.
     * @throws ScoreFormatError If the document is not a high score this
     * build can read.
     */
    [[nodiscard]] HighScore highScoreFromJson(
        const nlohmann::json &document);

    /**
     * @brief Write a high score to a stream.
     *
     * Indented, unlike a recorded replay: this is four lines somebody
     * may well want to read, where a recording is a long machine-written
     * log nobody opens.
     *
     * @param score What to write.
     * @param out The stream to write it to.
     */
    void writeHighScore(const HighScore &score, std::ostream &out);

    /**
     * @brief Read a high score from a stream.
     *
     * A stream rather than a path, so every refusal this can produce is
     * reachable from bytes in memory and provable with no fixture on
     * disk.
     *
     * @param in The stream to read from.
     * @return What it holds.
     * @throws ScoreFormatError If the stream is not valid JSON, or is
     * not a high score this build can read.
     */
    [[nodiscard]] HighScore readHighScore(std::istream &in);

    /**
     * @brief Fold a finished run into the best so far.
     *
     * The score decides, and the level comes along with whichever score
     * won: a run that scored more is the better run, and saying it got
     * less far would be reporting two different runs as one.
     *
     * @param best What has been achieved before.
     * @param run What this run finished on.
     * @return The record to keep.
     */
    [[nodiscard]] HighScore bestOf(
        const HighScore &best, const HighScore &run);

} // namespace antwika::tower_defence
