#pragma once

namespace antwika::input
{

    struct KeyModifiers final
    {
        bool shift = false;
        bool control = false;
        bool alt = false;
        bool super = false;

        [[nodiscard]] bool operator==(
            const KeyModifiers &other) const = default;
    };

}
