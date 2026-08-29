#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/Step.hpp>

namespace antwika::schema
{

    struct MigrationRow final
    {
        std::uint32_t fromVersion;

        std::uint32_t toVersion;

        std::string_view name;

        Apply apply;
    };

    void pushMigrations(
        MigrationList &migrations, std::span<const MigrationRow> rows);

    [[nodiscard]] Apply createEmptyArrays(std::vector<std::string_view> keys);

}
