#pragma once

#include <optional>
#include <string_view>

#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    /** @brief The flag naming a session's tick cap. */
    inline constexpr std::string_view kMaxTicksFlag = "--max-ticks";

    /**
     * @brief Read a --max-ticks value the way every capped app does.
     *
     * Zero means no cap at all, which is what somebody in front of a
     * real window asks for.  Anything unreadable leaves the fallback
     * in place, matching how these applications treat every other
     * malformed flag.  This parse used to be copied per application;
     * the copies had already begun to be the same code twice.
     *
     * @param value The flag's value, when the command line carried one.
     * @param fallback The cap when the flag is absent or unreadable.
     * @return The cap, or nothing to never stop.
     */
    [[nodiscard]] std::optional<antwika::time::Tick> maxTicksOf(
        std::optional<std::string_view> value,
        std::optional<antwika::time::Tick> fallback);

} // namespace antwika::app
