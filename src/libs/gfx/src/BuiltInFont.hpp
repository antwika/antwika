#pragma once

#include <antwika/font/Font.hpp>

namespace antwika::gfx::detail
{

    /**
     * @brief Get the one font this library draws text with.
     *
     * Parsed once, from bytes the build compiled in, so there is no
     * file to find and no failure a caller could be handed: the bytes
     * are the same on every machine and they are known to parse.
     * @return The font, which lives as long as the program does.
     */
    [[nodiscard]] const font::Font &builtInFont();

} // namespace antwika::gfx::detail
