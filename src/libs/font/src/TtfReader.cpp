#include "antwika/font/TtfReader.hpp"

#include <cstdint>
#include <istream>
#include <iterator>
#include <utility>
#include <vector>

#include "antwika/font/Font.hpp"

namespace antwika::font
{

    Font TtfReader::read(std::istream &inputStream) const
    {
        std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(inputStream),
            std::istreambuf_iterator<char>()};

        return Font{std::move(bytes)};
    }

}
