#include "antwika/assets/ShaderAssets.hpp"

#include <fstream>
#include <string>

#include <antwika/gfx/ShaderReader.hpp>
#include <antwika/io/AssetPath.hpp>

namespace antwika::assets
{

    gfx::ShaderSource getShaderSource(const std::string_view stem)
    {
        std::ifstream vertex(
            io::getAssetPath(std::string(stem) + ".vert"));
        std::ifstream fragment(
            io::getAssetPath(std::string(stem) + ".frag"));

        return gfx::ShaderReader().read(vertex, fragment);
    } // GCOVR_EXCL_LINE

}
