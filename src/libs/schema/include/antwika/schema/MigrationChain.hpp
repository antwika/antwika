#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <antwika/schema/IMigration.hpp>
#include <antwika/schema/SchemaVersion.hpp>

namespace antwika::schema
{

    using MigrationList = std::vector<std::shared_ptr<const IMigration>>;

    class MigrationChain final
    {
    public:
        MigrationChain(
            MigrationList migrations,
            std::uint32_t currentVersion,
            std::string versionKey = std::string(kSchemaVersionKey));

        [[nodiscard]] std::uint32_t getCurrentVersion() const noexcept;

        void migrate(nlohmann::json &document) const;

        void migrateFrom(
            nlohmann::json &record, std::uint32_t statedVersion) const;

        void requireReadable(std::uint32_t statedVersion) const;

    private:
        [[nodiscard]] const IMigration *stepFrom(
            std::uint32_t version) const noexcept;

        MigrationList migrations;
        std::uint32_t current;
        std::string versionKey;
    };

}
