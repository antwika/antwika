#pragma once

namespace antwika::ui
{

    template <typename... Arms>
    struct Overloaded final : Arms...
    {
        using Arms::operator()...;
    };

}
