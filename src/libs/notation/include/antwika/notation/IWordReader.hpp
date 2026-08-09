#pragma once

#include <cstddef>
#include <string_view>

#include <antwika/pattern/Controls.hpp>

namespace antwika::notation
{

    using antwika::pattern::Controls;

    class IWordReader
    {
    public:
        virtual ~IWordReader() = default;

        [[nodiscard]] virtual Controls read(
            std::string_view word, std::size_t at) const = 0;
    };

}
