#pragma once

#include <cstdint>

namespace antwika::pattern
{

    /**
     * @brief Names one thing a pattern's events carry.
     *
     * A scoped enum with no enumerators over `std::uint64_t`, following
     * antwika::ui::WidgetId and antwika::gfx::WindowId.
     *
     * **The caller chooses the values**, and this library never
     * interprets one.
     * That is what keeps the algebra ignorant of music: a pattern of
     * pitches and a pattern of filter cutoffs are the same type here,
     * differing only in an id the application gave meaning to.
     * Declaration order is deliberately not used instead, because an id
     * is what crosses back out into a sequencer and it has to keep
     * meaning the same thing when a score gains a parameter.
     */
    enum class ParamId : std::uint64_t
    {
    };

    /**
     * @brief The id nothing is named by.
     *
     * What a control carries when a caller did not name one.
     */
    inline constexpr ParamId kNoParam{0};

} // namespace antwika::pattern
