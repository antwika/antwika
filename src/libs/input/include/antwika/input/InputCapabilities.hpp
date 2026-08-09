#pragma once

namespace antwika::input
{

    struct InputCapabilities final
    {
        bool keyboard = false;
        bool pointer = false;

        [[nodiscard]] bool operator==(
            const InputCapabilities &other) const = default;
    };

}
