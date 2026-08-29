#pragma once

namespace antwika::map
{

    struct Settings final
    {
        bool lighting = true;

        bool cornersJoined = false;

        [[nodiscard]] bool operator==(const Settings &other) const
            = default;
    };

}
