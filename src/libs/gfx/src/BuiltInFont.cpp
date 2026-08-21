#include "BuiltInFont.hpp"

#include <cstdint>
#include <vector>

#include <antwika/font/Font.hpp>

#include "BuiltInFontBytes.hpp"

namespace antwika::gfx::detail
{

    const font::Font &builtInFont()
    {
        static const font::Font font{std::vector<std::uint8_t>(
            kBuiltInFontBytes,
            kBuiltInFontBytes + kBuiltInFontBytesSize)}; // GCOVR_EXCL_LINE

        return font;
    }

}
