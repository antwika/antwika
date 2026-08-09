#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    using antwika::replay::MigrationChain;

    struct PlayerOptions final
    {
        KeyBindings bindings{};

        antwika::i18n::Locale locale{antwika::i18n::kDefaultLocale};

        KeyboardLayout keyboard{kDefaultKeyboardLayout};

        [[nodiscard]] bool operator==(
            const PlayerOptions &other) const = default;
    };

    inline constexpr std::string_view kOptionsMagic =
        "antwika-game-options";

    inline constexpr std::uint32_t kOptionsFormatVersion = 3;

    inline constexpr std::string_view kLocaleKey = "locale";

    inline constexpr std::string_view kKeyboardKey = "keyboard";

    [[nodiscard]] MigrationChain standardOptionsMigrations();

    [[nodiscard]] nlohmann::json optionsToJson(
        const PlayerOptions &options);

    [[nodiscard]] PlayerOptions optionsFromJson(
        const nlohmann::json &document);

    void writeOptions(const PlayerOptions &options, std::ostream &out);

    [[nodiscard]] PlayerOptions readOptions(std::istream &in);

    void saveOptionsFile(
        const PlayerOptions &options, const std::string &path);

    [[nodiscard]] PlayerOptions loadOptionsFileOrDefaults(
        const std::string &path);

    void saveOptionsFileIfNamed(
        const PlayerOptions &options,
        const std::optional<std::string> &path);

    struct MachineOptions final
    {
        std::optional<KeyBindings> bindings{};

        std::optional<antwika::i18n::Locale> locale{};

        std::optional<KeyboardLayout> keyboard{};

        std::optional<std::string> path{};

        [[nodiscard]] bool operator==(
            const MachineOptions &other) const = default;
    };

    [[nodiscard]] MachineOptions machineOptionsFor(
        bool replaying, const std::string &path);

}
