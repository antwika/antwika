#pragma once

#include <span>
#include <string>
#include <string_view>

namespace antwika::i18n
{

    /**
     * @brief Replace `{0}`-style placeholders with positional arguments.
     *
     * Positional rather than named, because the argument order is the one
     * thing a translator legitimately changes: Swedish may want the number
     * where English wants the noun, and `{0}` lets the catalogue say so
     * without the calling code knowing.
     *
     * A `{` that does not open a run of decimal digits closed by `}`, or
     * that names an argument nobody supplied, is copied out as written.
     * That keeps the function total -- there is no such thing as a pattern
     * it refuses -- and leaves a mis-numbered placeholder visible in the
     * text instead of silently blank.
     * A literal `{0}` is therefore not expressible; no message needs one.
     *
     * @param pattern The text to substitute into.
     * @param args The arguments, addressed by their index in this span.
     * @return The substituted text.
     */
    [[nodiscard]] std::string substitute(
        std::string_view pattern, std::span<const std::string_view> args);

} // namespace antwika::i18n
