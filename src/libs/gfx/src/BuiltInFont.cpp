#include "BuiltInFont.hpp"

#include <cstdint>
#include <vector>

#include <antwika/font/Font.hpp>

#include "BuiltInFontBytes.hpp"

namespace antwika::gfx::detail
{

    const font::Font &builtInFont()
    {
        // Parsed on the first line of text drawn, and kept after that.
        // A Font owns a rasteriser over the bytes it was handed.
        // Re-reading a table directory per frame is not frame work.
        // The excluded line holds the guard's throw and abort edges.
        // That is the exclusion antwika::replay's validators carry.
        static const font::Font font{std::vector<std::uint8_t>(
            kBuiltInFontBytes,
            kBuiltInFontBytes + kBuiltInFontBytesSize)}; // GCOVR_EXCL_LINE

        return font;
    }

} // namespace antwika::gfx::detail
