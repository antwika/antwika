#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string_view>

namespace antwika::replay
{

    inline constexpr std::string_view kSchemaVersionKey = "version";

    inline constexpr std::uint32_t kUnversionedDocumentVersion = 1;

    inline constexpr std::uint32_t kReplayDocumentVersion = 2;

    inline constexpr std::uint32_t kTickEventSchemaVersion = 1;

    [[nodiscard]] std::uint32_t documentVersion(
        const nlohmann::json &document,
        std::string_view versionKey = kSchemaVersionKey);

}
