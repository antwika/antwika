#include "antwika/schema/MigrationRow.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <utility>

namespace antwika::schema
{

    void pushMigrations(
        MigrationList &migrations, const std::span<const MigrationRow> rows)
    {
        for (const auto &row : rows)
        {
            migrations.push_back(getMigration(
                row.fromVersion,
                row.toVersion,
                std::string(row.name),
                row.apply));
        }
    }

    Apply createEmptyArrays(std::vector<std::string_view> keys)
    {
        return [keys = std::move(keys)](nlohmann::json &document)
        {
            for (const auto key : keys)
            {
                document[std::string(key)] = nlohmann::json::array();
            }
        };
    } // GCOVR_EXCL_LINE

}
