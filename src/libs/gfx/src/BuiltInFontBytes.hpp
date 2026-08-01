#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::gfx::detail
{

    // The bytes of assets/fonts/RobotoMono-Regular.ttf, compiled in.
    // antwika_embed_binary() writes the definitions at configure time.
    // So what is checked in is the font, never a copy of it in hex.
    // These declarations are the other half of that generated source.
    // A disagreement between the two is a link error rather than a run.
    extern const std::uint8_t kBuiltInFontBytes[];
    extern const std::size_t kBuiltInFontBytesSize;

} // namespace antwika::gfx::detail
