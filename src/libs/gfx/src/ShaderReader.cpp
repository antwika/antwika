#include "antwika/gfx/ShaderReader.hpp"

#include <istream>
#include <iterator>
#include <string>

#include "antwika/gfx/GfxError.hpp"

namespace antwika::gfx
{

    std::string ShaderReader::readAll(std::istream &inputStream) const
    {
        std::string text{
            std::istreambuf_iterator<char>(inputStream),
            std::istreambuf_iterator<char>()};

        if (inputStream.bad())
        {
            throw GfxError("gfx: could not read the shader text");
        }

        if (text.empty())
        {
            throw GfxError("gfx: the shader text is empty");
        }

        return text;
    }

    ShaderSource ShaderReader::read(
        std::istream &vertex, std::istream &fragment) const
    {
        ShaderSource source;

        source.vertex = readAll(vertex);
        source.fragment = readAll(fragment);

        return source;
    }

}
