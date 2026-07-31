#include "antwika/replay/MigrationChain.hpp"

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
        for (const auto &migration : this->migrations)
        {
            if (migration->toVersion() != migration->fromVersion() + 1)
            {
                throw SchemaVersionError(std::format(
                    "antwika::replay: migration \"{}\" is not a single "
                    "step: it reads version {} and produces version {}",
                    migration->name(),
                    migration->fromVersion(),
                    migration->toVersion()));
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

    void MigrationChain::migrate(nlohmann::json &document) const
    {
        if (!document.is_object())
        {
            return; // The caller's schema refuses it, and says why.
        }

        auto version = documentVersion(document, versionKey);
        if (version > current)
        {
            throw SchemaVersionError(std::format(
                "antwika::replay: this document states schema version "
                "{}, and this build reads up to version {}; it was "
                "written by a newer release",
                version,
                current));
        }

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
                    documentVersion(document, versionKey),
                    version,
                    current));
            }

            step->apply(document);
            version = step->toVersion();
        }

        // Stamping the version is the chain's job, never a migration's.
        // It is also what gives a document that stated none one.
        document[versionKey] = version;
    }

} // namespace antwika::replay
