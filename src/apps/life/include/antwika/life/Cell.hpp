#pragma once

namespace antwika::life
{

    struct Cell final
    {
        bool alive{};

        bool operator==(const Cell &other) const = default;
    };

}
