#pragma once

#include <string_view>

#include <antwika/gfx/ShaderSource.hpp>

namespace antwika::assets
{

    [[nodiscard]] gfx::ShaderSource getShaderSource(
        std::string_view stem);

}
