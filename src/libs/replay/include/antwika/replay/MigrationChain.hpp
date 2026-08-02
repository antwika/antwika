#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/SchemaVersion.hpp>

namespace antwika::replay
{

    /**
     * @brief The migrations a chain is built from, oldest first.
     */
    using MigrationList = std::vector<std::shared_ptr<const IMigration>>;

    /**
     * @brief Brings a parsed document up to the current version of its
     * schema by applying one single-step migration after another.
     *
     * Generic over the document on purpose: it operates on an
     * nlohmann::json and a version key and knows nothing about replays,
     * so a save file uses the same mechanism.
     * A caller builds its own list, names its own current version, and
     * hands both to a chain:
     *
     *     MigrationList migrations;
     *     migrations.push_back(std::make_shared<SaveV1ToV2>());
     *     migrations.push_back(std::make_shared<SaveV2ToV3>());
     *     const MigrationChain chain(std::move(migrations), 3);
     *     chain.migrate(parsed);
     *
     * There is no registry and no global state: a chain is constructed
     * and injected, so two document kinds in one process cannot see each
     * other's migrations.
     * standardReplayMigrations() is the factory for the replay
     * document's own chain.
     *
     * The right place to run one is between parsing and validating:
     *
     *     parse -> read version -> migrate -> validate -> decode
     *
     * Validating afterwards is what lets exactly one schema exist,
     * describing the current version, rather than one per revision.
     */
    class MigrationChain final
    {
    public:
        /**
         * @brief Construct a chain.
         * @param migrations The single-step migrations, in any order;
         * the chain looks each step up by the version it reads.
         * @param currentVersion The version a migrated document ends up
         * at, e.g. kReplayDocumentVersion.
         * @param versionKey The member the document states its version
         * in; the shared kSchemaVersionKey unless a format says
         * otherwise.
         * @throws SchemaVersionError If any migration is not a single
         * step, i.e. its toVersion() is not its fromVersion() + 1, or
         * two of them read the same fromVersion().
         *
         * Those checks are in the constructor rather than in migrate()
         * because a chain that cannot terminate is a fact about the
         * chain, not about any one document, and finding it out while
         * loading somebody's file is too late.
         * The duplicate check is there for the same reason: the lookup
         * takes the first migration reading a version and never looks
         * further, so a second one would be applied by nothing and
         * reported by nothing either.
         */
        MigrationChain(
            MigrationList migrations,
            std::uint32_t currentVersion,
            std::string versionKey = std::string(kSchemaVersionKey));

        /**
         * @brief The version migrate() brings a document to.
         */
        [[nodiscard]] std::uint32_t currentVersion() const noexcept;

        /**
         * @brief Bring a document up to the current version, in place.
         * @param document The parsed document; left untouched if it is
         * not a JSON object, since the caller's schema refuses that and
         * says why better than this could.
         * @throws SchemaVersionError If the document states a version
         * that is not a whole number, states one newer than
         * currentVersion(), or reaches a version no migration reads --
         * a gap in the chain.
         *
         * A document with no version member is version 1, and comes out
         * stating the version it was brought to, so what follows always
         * sees a document that says which revision it is.
         */
        void migrate(nlohmann::json &document) const;

        /**
         * @brief Bring one record of a line-oriented file up to the
         * current version, in place.
         * @param record The parsed record; left untouched if it is not a
         * JSON object, exactly as migrate() leaves a document.
         * @param statedVersion The version its file's header stated.
         * @throws SchemaVersionError If that version is newer than
         * currentVersion(), or reaches a version no migration reads.
         *
         * The same five stages as migrate(), with the version read from
         * the file's header instead of from the record.
         * A record states no version of its own and comes out stating
         * none: one number per file, not one per line, because a record
         * repeats thousands of times and cannot disagree with the header
         * that opens the file it is in.
         */
        void migrateFrom(
            nlohmann::json &record, std::uint32_t statedVersion) const;

        /**
         * @brief Refuse a version this build cannot reach the current
         * one from.
         * @param statedVersion The version a file or a document states.
         * @throws SchemaVersionError If it is newer than
         * currentVersion().
         *
         * Public because a line-oriented file states its version once,
         * in its header, and wants that answered before it reads a
         * single record -- including when it holds none at all.
         */
        void requireReadable(std::uint32_t statedVersion) const;

    private:
        [[nodiscard]] const IMigration *stepFrom(
            std::uint32_t version) const noexcept;

        MigrationList migrations;
        std::uint32_t current;
        std::string versionKey;
    };

} // namespace antwika::replay
