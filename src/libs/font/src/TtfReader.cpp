#include "antwika/font/TtfReader.hpp"

#include <cstdint>
#include <istream>
#include <iterator>
#include <utility>
#include <vector>

#include "antwika/font/Font.hpp"

namespace antwika::font
{

    Font TtfReader::read(std::istream &in) const
    {
        std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};

        // A prvalue, so this is the returned Font rather than a copy.
        // That is what lets Font delete every copy and move it has.
        return Font{std::move(bytes)};
    }

} // namespace antwika::font
