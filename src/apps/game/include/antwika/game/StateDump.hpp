#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-game-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 2;

    struct StateDump final
    {
        SaveGame save;

        bool paused = false;

        std::optional<BuildTool> tool = std::nullopt;

        MapView view = MapView::Normal;

        antwika::i18n::Locale locale = antwika::i18n::kDefaultLocale;

        [[nodiscard]] bool operator==(
            const StateDump &other) const = default;
    };

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

}
