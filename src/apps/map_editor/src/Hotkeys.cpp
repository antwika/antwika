#include "antwika/map_editor/Hotkeys.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/input/InputError.hpp>

namespace antwika::map_editor
{

    namespace
    {
        using antwika::input::Key;

        struct ActionInfo final
        {
            std::string_view name;
            std::string_view label;
        };

        constexpr std::array<ActionInfo, kHotkeyActionCount>
            kActionInfo{{
                {.name = "raiseHeight", .label = "level up"},
                {.name = "lowerHeight", .label = "level down"},
                {.name = "bridge", .label = "bridge"},
                {.name = "light", .label = "light"},
                {.name = "undo", .label = "undo"},
                {.name = "redo", .label = "redo"},
                {.name = "save", .label = "save"},
                {.name = "reload", .label = "reload"},
                {.name = "generate", .label = "generate"},
                {.name = "validator", .label = "validator"},
                {.name = "placeTransition",
                 .label = "place transition"},
                {.name = "placeNpc", .label = "place npc"},
                {.name = "placeKey", .label = "place key"},
                {.name = "deleteEntities",
                 .label = "delete entities"},
                {.name = "stampCorner", .label = "stamp corner"},
                {.name = "stampCopy", .label = "stamp copy"},
                {.name = "stampPaste", .label = "stamp paste"},
                {.name = "drawColor", .label = "draw color"},
                {.name = "playtest", .label = "playtest"},
                {.name = "fullscreen", .label = "fullscreen"},
                {.name = "picker", .label = "sprite picker"},
            }};
    }

    std::string_view toString(const HotkeyAction action)
    {
        return kActionInfo[enums::index(action)].name;
    }

    std::string_view hotkeyLabel(const HotkeyAction action)
    {
        return kActionInfo[enums::index(action)].label;
    }

    bool bindableHotkey(const input::Key key)
    {
        if (key >= Key::A && key <= Key::Z)
        {
            return true;
        }

        if (key >= Key::F1 && key <= Key::F12)
        {
            return true;
        }

        switch (key)
        {
            case Key::Minus:
            case Key::Equal:
            case Key::LeftBracket:
            case Key::RightBracket:
            case Key::Backslash:
            case Key::Semicolon:
            case Key::Apostrophe:
            case Key::Comma:
            case Key::Period:
            case Key::Slash:
                return true;
            default:
                return false;
        }
    }

    std::optional<HotkeyAction> actionOfKey(
        const HotkeyBindings &bindings, const input::Key key)
    {
        for (const auto action : enums::kAll<HotkeyAction>)
        {
            if (bindings[enums::index(action)] == key)
            {
                return action;
            }
        }

        return std::nullopt;
    }

    std::string_view keyCaption(const input::Key key)
    {
        switch (key)
        {
            case Key::Minus:
                return "-";
            case Key::Equal:
                return "=";
            case Key::LeftBracket:
                return "[";
            case Key::RightBracket:
                return "]";
            case Key::Backslash:
                return "\\";
            case Key::Semicolon:
                return ";";
            case Key::Apostrophe:
                return "'";
            case Key::Comma:
                return ",";
            case Key::Period:
                return ".";
            case Key::Slash:
                return "/";
            default:
                return input::toString(key);
        }
    }

    HotkeyBindings hotkeysFromConfig(
        const std::map<std::string, std::string> &entries)
    {
        auto bindings = defaultHotkeyBindings();
        const auto defaults = defaultHotkeyBindings();

        for (const auto action : enums::kAll<HotkeyAction>)
        {
            const auto found = entries.find(
                std::string(toString(action)));

            if (found == entries.end())
            {
                continue;
            }

            try
            {
                const auto key =
                    input::keyFromString(found->second);

                if (bindableHotkey(key))
                {
                    bindings[enums::index(action)] = key;
                }
            }
            catch (const input::InputError &) // GCOVR_EXCL_LINE
            {
            }
        }

        for (std::size_t at = 0; at < kHotkeyActionCount; ++at)
        {
            for (std::size_t earlier = 0; earlier < at; ++earlier)
            {
                if (bindings[earlier] == bindings[at])
                {
                    bindings[at] = defaults[at];
                    break;
                }
            }
        }

        return bindings;
    }

    std::map<std::string, std::string> hotkeysToConfig(
        const HotkeyBindings &bindings)
    {
        std::map<std::string, std::string> entries;

        for (const auto action : enums::kAll<HotkeyAction>)
        {
            entries[std::string(toString(action))] = std::string(
                input::toString(bindings[enums::index(action)]));
        }

        return entries;
    } // GCOVR_EXCL_LINE

}
