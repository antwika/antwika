#pragma once

#include <cstdint>
#include <string_view>

#include <nlohmann/json.hpp>

namespace antwika::replay
{

    /**
     * @brief One step of a document's history: everything that has to
     * change to turn a document at version N into the same document at
     * version N+1.
     *
     * A migration is always a single step.
     * Writing one straight from an old version to the current one
     * instead would mean a new migration for every older version at
     * every bump -- quadratic in the number of bumps, and each of them
     * another place to get it wrong.
     * MigrationChain composes the single steps, so the number of
     * migrations stays linear: one per bump, forever.
     *
     * Nothing here is replay-specific.
     * A save file, or any other versioned JSON document, implements this
     * interface and hands its migrations to a MigrationChain of its own.
     */
    class IMigration
    {
    public:
        IMigration() = default;
        IMigration(const IMigration &) = delete;
        IMigration(IMigration &&) = delete;
        IMigration &operator=(const IMigration &) = delete;
        IMigration &operator=(IMigration &&) = delete;
        virtual ~IMigration() = default;

        /**
         * @brief The version this migration reads.
         */
        [[nodiscard]] virtual std::uint32_t fromVersion() const
            noexcept = 0;

        /**
         * @brief The version this migration produces.
         *
         * Must be fromVersion() + 1; MigrationChain refuses a chain
         * whose steps say otherwise rather than trusting them.
         */
        [[nodiscard]] virtual std::uint32_t toVersion() const
            noexcept = 0;

        /**
         * @brief What this migration is called, for the message a
         * failure carries.
         */
        [[nodiscard]] virtual std::string_view name() const noexcept = 0;

        /**
         * @brief Rewrite a document at fromVersion() into one at
         * toVersion().
         * @param document The parsed document, modified in place.
         *
         * Must not touch the version member: MigrationChain stamps that
         * itself, so a migration cannot leave a document claiming to be
         * something it is not.
         */
        virtual void apply(nlohmann::json &document) const = 0;
    };

} // namespace antwika::replay
