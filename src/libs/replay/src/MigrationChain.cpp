#include "antwika/replay/MigrationChain.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <utility>

#include <antwika/replay/SchemaVersionError.hpp>

namespace antwika::replay
{

    MigrationChain::MigrationChain(
        MigrationList migrations,
        std::uint32_t currentVersion,
        std::string versionKey)
        : migrations(std::move(migrations)),
          current(currentVersion),
          versionKey(std::move(versionKey))
    {
        for (std::size_t i = 0; i < this->migrations.size(); ++i)
        {
            const auto &migration = this->migrations[i];
            if (migration->toVersion() != migration->fromVersion() + 1)
            {
                throw SchemaVersionError(std::format(
                    "antwika::replay: migration \"{}\" is not a single "
                    "step: it reads version {} and produces version {}",
                    migration->name(),
                    migration->fromVersion(),
                    migration->toVersion()));
            }

            // stepFrom() takes the first match and never looks further.
            // So a second reader of one version is applied by nothing.
            // And nothing anywhere says the chain lost a step.
            // Refused here for the reason the check above is.
            // A chain losing a step is a fact about the chain.
            for (std::size_t j = 0; j < i; ++j)
            {
                const auto &earlier = this->migrations[j];
                if (earlier->fromVersion() != migration->fromVersion())
                {
                    continue;
                }

                throw SchemaVersionError(std::format(
                    "antwika::replay: migrations \"{}\" and \"{}\" both "
                    "read version {}; one of them would be applied and "
                    "the other silently shadowed",
                    earlier->name(),
                    migration->name(),
                    migration->fromVersion()));
            }
        }
    }

    std::uint32_t MigrationChain::currentVersion() const noexcept
    {
        return current;
    }

    const IMigration *MigrationChain::stepFrom(
        std::uint32_t version) const noexcept
    {
        for (const auto &migration : migrations)
        {
            if (migration->fromVersion() == version)
            {
                return migration.get();
            }
        }
        return nullptr;
    }

    void MigrationChain::requireReadable(
        const std::uint32_t statedVersion) const
    {
        if (statedVersion > current)
        {
            throw SchemaVersionError(std::format(
                "antwika::replay: this document states schema version "
                "{}, and this build reads up to version {}; it was "
                "written by a newer release",
                statedVersion,
                current));
        }

        // The other end, refused up front and blaming the document.
        // Left to the migration walk, a sub-oldest version either
        // loaded silently (nothing to migrate in an empty file) or
        // surfaced as a "gap" message blaming this build's chain for
        // a document no release of this software ever wrote.
        auto oldest = current;

        for (const auto &migration : migrations)
        {
            oldest = std::min(oldest, migration->fromVersion());
        }

        if (statedVersion < oldest)
        {
            throw SchemaVersionError(std::format(
                "antwika::replay: this document states schema version "
                "{}, and this build reads versions {} through {}; no "
                "release ever wrote that version",
                statedVersion,
                oldest,
                current));
        }
    }

    void MigrationChain::migrate(nlohmann::json &document) const
    {
        if (!document.is_object())
        {
            return; // The caller's schema refuses it, and says why.
        }

        migrateFrom(document, documentVersion(document, versionKey));

        // Stamping the version is the chain's job, never a migration's.
        // It is also what gives a document that stated none one.
        document[versionKey] = current;
    }

    void MigrationChain::migrateFrom(
        nlohmann::json &record, const std::uint32_t statedVersion) const
    {
        requireReadable(statedVersion);

        if (!record.is_object())
        {
            return; // The caller's schema refuses it, and says why.
        }

        // The gap message names the stated version, held here.
        // Re-reading it would rest on every migration obeying the rule.
        // That rule is IMigration's, and this chain does not enforce it.
        // A record carries no version to re-read in any case.
        auto version = statedVersion;
        while (version < current)
        {
            const IMigration *const step = stepFrom(version);
            if (step == nullptr)
            {
                throw SchemaVersionError(std::format(
                    "antwika::replay: this document states schema "
                    "version {}, and no migration reads version {} on "
                    "the way to version {}; the migration chain has a "
                    "gap",
                    statedVersion,
                    version,
                    current));
            }

            step->apply(record);
            version = step->toVersion();
        }
    }

} // namespace antwika::replay
