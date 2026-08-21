#pragma once

#include <iosfwd>
#include <string>

#include "antwika/gfx/ShaderSource.hpp"

namespace antwika::gfx
{

    class ShaderReader final
    {
    public:
        [[nodiscard]] std::string readAll(std::istream &inputStream) const;

        [[nodiscard]] ShaderSource read(
            std::istream &vertex, std::istream &fragment) const;
    };

}
