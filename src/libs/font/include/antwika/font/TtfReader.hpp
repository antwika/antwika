#pragma once

#include <iosfwd>

#include "antwika/font/Font.hpp"

namespace antwika::font
{

    class TtfReader final
    {
    public:
        [[nodiscard]] Font read(std::istream &in) const;
    };

}
