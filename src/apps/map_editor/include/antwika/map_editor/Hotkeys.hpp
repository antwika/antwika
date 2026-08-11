#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/input/Key.hpp>

namespace antwika::map_editor
{

    enum class HotkeyAction : std::uint8_t
    {
        RaiseHeight = 0,
        LowerHeight,
        Bridge,
        Light,
        Undo,
        Redo,
        Save,
        Reload,
        Generate,
        Validator,
        PlaceTransition,
        PlaceNpc,
        PlaceKey,
        DeleteEntities,
        StampCorner,
        StampCopy,
        StampPaste,
        DrawColor,
        Playtest,
        Fullscreen,
        Picker,
    };

    [[nodiscard]] constexpr HotkeyAction enumBound(
        HotkeyAction) noexcept
    {
        return HotkeyAction::Picker;
    }

    inline constexpr std::size_t kHotkeyActionCount =
        enums::kCount<HotkeyAction>;

    using HotkeyBindings =
        std::array<input::Key, kHotkeyActionCount>;

    [[nodiscard]] constexpr HotkeyBindings
    defaultHotkeyBindings() noexcept
    {
        using input::Key;

        return HotkeyBindings{
            Key::E,
            Key::Q,
            Key::B,
            Key::L,
            Key::U,
            Key::R,
            Key::S,
            Key::O,
            Key::G,
            Key::V,
            Key::T,
            Key::N,
            Key::K,
            Key::X,
            Key::LeftBracket,
            Key::RightBracket,
            Key::P,
            Key::C,
            Key::F5,
            Key::F10,
            Key::I};
    }

    /**
     * @brief The stable name an action keeps in the config file.
     */
    [[nodiscard]] std::string_view toString(HotkeyAction action);

    /**
     * @brief The label an action shows in the keys dialog.
     */
    [[nodiscard]] std::string_view hotkeyLabel(HotkeyAction action);

    /**
     * @brief Whether a key may hold a hotkey binding.
     *
     * Ensures: letters, function keys, and punctuation other than
     *          grave qualify, while digits, Tab, Escape, grave, and
     *          the arrows stay reserved.
     */
    [[nodiscard]] bool bindableHotkey(input::Key key);

    /**
     * @brief The action a key is bound to, if any.
     *
     * Ensures: a key held by several actions resolves to the first
     *          in declaration order, so one key fires one action.
     */
    [[nodiscard]] std::optional<HotkeyAction> actionOfKey(
        const HotkeyBindings &bindings, input::Key key);

    /**
     * @brief The short caption a key shows in labels and hints.
     */
    [[nodiscard]] std::string_view keyCaption(input::Key key);

    /**
     * @brief Rebuilds the bindings from config-file entries.
     *
     * Ensures: unknown action names, unknown key names, and reserved
     *          keys fall back to the defaults, and a key claimed
     *          twice keeps its first action.
     */
    [[nodiscard]] HotkeyBindings hotkeysFromConfig(
        const std::map<std::string, std::string> &entries);

    /**
     * @brief The config-file entries matching the bindings.
     */
    [[nodiscard]] std::map<std::string, std::string> hotkeysToConfig(
        const HotkeyBindings &bindings);

}
