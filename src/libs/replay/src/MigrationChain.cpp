#include "antwika/replay/MigrationChain.hpp"

#include <nlohmann/json.hpp>

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
            return;
        }

        migrateFrom(document, documentVersion(document, versionKey));

        document[versionKey] = current;
    }

    void MigrationChain::migrateFrom(
        nlohmann::json &record, const std::uint32_t statedVersion) const
    {
        requireReadable(statedVersion);

        if (!record.is_object())
        {
            return;
        }

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

}
