#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace antwika::ui
{

    enum class Key : std::uint8_t
    {
        FocusNext = 0,

        FocusPrevious,

        Activate,

        Backspace,

        Cancel,

        MoveLeft,

        MoveRight,

        MoveUp,

        MoveDown,

        MoveLineStart,

        MoveLineEnd,

        SelectLeft,

        SelectRight,

        SelectUp,

        SelectDown,

        SelectLineStart,

        SelectLineEnd,

        SelectAll,

        Delete,

        Copy,

        Cut,

        Character,
    };

    struct Keyboard final
    {
        std::vector<Key> keys{};

        std::string_view typedText{};

        [[nodiscard]] bool operator==(const Keyboard &other) const =
            default;
    };

}
