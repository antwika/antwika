#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/companion/CompanionMemory.hpp"

namespace antwika::companion
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kSaveMagic = "antwika-companion";

    inline constexpr std::uint32_t kSaveFormatVersion = 3;

    [[nodiscard]] MigrationChain standardPetMigrations();

    [[nodiscard]] nlohmann::json companionMemoryToJson(
        const CompanionMemory &memory);

    [[nodiscard]] CompanionMemory companionMemoryFromJson(
        const nlohmann::json &document);

    void writeCompanionMemory(
        const CompanionMemory &memory, std::ostream &out);

    [[nodiscard]] CompanionMemory readCompanionMemory(std::istream &in);

}
