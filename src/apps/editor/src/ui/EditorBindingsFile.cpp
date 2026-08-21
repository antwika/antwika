#include <nlohmann/json.hpp>

#include <array>
#include <fstream>
#include <stdexcept>

#include <antwika/io/SafeWrite.hpp>

#include "antwika/editor/ui/EditorBindings.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::array<
            std::string_view,
            96>
            kKeyNames{
                "a",
                "b",
                "c",
                "d",
                "e",
                "f",
                "g",
                "h",
                "i",
                "j",
                "k",
                "l",
                "m",
                "n",
                "o",
                "p",
                "q",
                "r",
                "s",
                "t",
                "u",
                "v",
                "w",
                "x",
                "y",
                "z",
                "0",
                "1",
                "2",
                "3",
                "4",
                "5",
                "6",
                "7",
                "8",
                "9",
                "keypad 0",
                "keypad 1",
                "keypad 2",
                "keypad 3",
                "keypad 4",
                "keypad 5",
                "keypad 6",
                "keypad 7",
                "keypad 8",
                "keypad 9",
                "keypad add",
                "keypad subtract",
                "f1",
                "f2",
                "f3",
                "f4",
                "f5",
                "f6",
                "f7",
                "f8",
                "f9",
                "f10",
                "f11",
                "f12",
                "left",
                "right",
                "up",
                "down",
                "escape",
                "enter",
                "space",
                "tab",
                "backspace",
                "delete",
                "insert",
                "home",
                "end",
                "page up",
                "page down",
                "minus",
                "equal",
                "left bracket",
                "right bracket",
                "backslash",
                "semicolon",
                "apostrophe",
                "grave",
                "comma",
                "period",
                "slash",
                "intl backslash",
                "caps lock",
                "left shift",
                "right shift",
                "left control",
                "right control",
                "left alt",
                "right alt",
                "left super",
                "right super"};

        constexpr std::string_view kKeyKey = "key";

        constexpr std::string_view kCtrlKey = "ctrl";

        constexpr std::string_view kShiftKey = "shift";

        constexpr std::string_view kAltKey = "alt";

        [[nodiscard]] std::optional<input::Key> keyNamed(
            const std::string &text)
        {
            for (std::size_t index = 0; index < kKeyNames.size();
                 ++index)
            {
                if (kKeyNames.at(index) == text)
                {
                    return static_cast<input::Key>(index);
                }
            }

            return std::nullopt;
        }
    }

    std::string_view keyName(const input::Key key)
    {
        const auto keyIndex = static_cast<std::size_t>(key);

        return keyIndex < kKeyNames.size() ? kKeyNames.at(keyIndex) : "";
    }

    std::string chordName(const std::optional<Chord> &chord)
    {
        if (!chord.has_value())
        {
            return "-";
        }

        std::string chordText;

        if (chord->ctrl)
        {
            chordText += "ctrl+";
        }

        if (chord->shift)
        {
            chordText += "shift+";
        }

        if (chord->alt)
        {
            chordText += "alt+";
        }

        return chordText + std::string(keyName(chord->key));
    } // GCOVR_EXCL_LINE

    void saveChords(
        const KeyBindings &keyBindings, const std::string &path)
    {
        nlohmann::json document;

        for (const auto &[act, chord] : keyBindings)
        {
            auto &entry =
                document[std::string(actionKey(act))];

            if (!chord.has_value())
            {
                entry = nullptr;

                continue;
            }

            entry[std::string(kKeyKey)] =
                std::string(keyName(chord->key));
            entry[std::string(kCtrlKey)] = chord->ctrl;
            entry[std::string(kShiftKey)] = chord->shift;
            entry[std::string(kAltKey)] = chord->alt;
        }

        const auto writingPath = io::writingPathFor(path);

        {
            std::ofstream outputStream(writingPath);

            outputStream << document.dump(2) << '\n';
        }

        io::putInPlaceKeepingBackup<std::runtime_error>(
            writingPath, path, "the key bindings");
    }

    KeyBindings loadChords(const std::string &path)
    {
        auto chords = defaultChords();
        std::ifstream inputStream(path);

        if (!inputStream)
        {
            return chords;
        }

        nlohmann::json document;

        try
        {
            inputStream >> document;
        }
        catch (const nlohmann::json::exception &)
        {
            return chords;
        }

        if (!document.is_object())
        {
            return chords;
        }

        for (const auto act : allActions())
        {
            const auto token = std::string(actionKey(act));

            if (!document.contains(token))
            {
                continue;
            }

            const auto &entry = document[token];

            if (entry.is_null())
            {
                chords[act] = std::nullopt;

                continue;
            }

            if (!entry.is_object()
                || !entry.contains(std::string(kKeyKey))
                || !entry[std::string(kKeyKey)].is_string())
            {
                continue;
            }

            const auto key = keyNamed(
                entry[std::string(kKeyKey)]
                    .get<std::string>());

            if (!key.has_value())
            {
                continue;
            }

            const auto flag = [&entry](
                                  const std::string_view name)
            {
                return entry.contains(std::string(name))
                       && entry[std::string(name)].is_boolean()
                       && entry[std::string(name)].get<bool>();
            };

            chords[act] = Chord{
                .key = *key,
                .ctrl = flag(kCtrlKey),
                .shift = flag(kShiftKey),
                .alt = flag(kAltKey)};
        }

        return chords;
    } // GCOVR_EXCL_LINE

}
