#pragma once

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

namespace antwika::tower_defence
{

    using antwika::replay::MigrationChain;

    struct HighScore final
    {
        std::uint64_t bestScore = 0;

        std::size_t bestLevel = 0;

        [[nodiscard]] bool operator==(const HighScore &) const = default;
    };

    inline constexpr std::string_view kScoreMagic =
        "antwika-tower-defence-score";

    inline constexpr std::uint32_t kScoreFormatVersion = 1;

    [[nodiscard]] MigrationChain standardScoreMigrations();

    [[nodiscard]] nlohmann::json highScoreToJson(const HighScore &score);

    [[nodiscard]] HighScore highScoreFromJson(
        const nlohmann::json &document);

    void writeHighScore(const HighScore &score, std::ostream &out);

    [[nodiscard]] HighScore readHighScore(std::istream &in);

    [[nodiscard]] HighScore bestOf(
        const HighScore &best, const HighScore &run);

}
