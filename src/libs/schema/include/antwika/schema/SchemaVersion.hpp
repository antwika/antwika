#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string_view>

namespace antwika::schema
{

    inline constexpr std::string_view kSchemaVersionKey = "version";

    inline constexpr std::uint32_t kImplicitDocumentVersion = 1;

    [[nodiscard]] std::uint32_t getDocumentVersion(
        const nlohmann::json &document,
        std::string_view versionKey = kSchemaVersionKey);

}
