#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/SchemaVersion.hpp>

namespace antwika::replay
{

    using MigrationList = std::vector<std::shared_ptr<const IMigration>>;

    class MigrationChain final
    {
    public:
        MigrationChain(
            MigrationList migrations,
            std::uint32_t currentVersion,
            std::string versionKey = std::string(kSchemaVersionKey));

        [[nodiscard]] std::uint32_t currentVersion() const noexcept;

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
