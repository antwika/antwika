#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::music_editor::detail
{

    // Twelve semitones to the octave.
    // The only music theory this application contains, held once.
    // TrackPreset divides by it in double; the cast happens there.
    inline constexpr std::int32_t kSemitonesPerOctave = 12;

    /**
     * @brief Strip leading and trailing blanks off a view.
     *
     * The one copy: Score cuts lines with it and VoiceChain cuts call
     * segments with it, and the two must never drift apart on what a
     * blank is.
     *
     * @param text The view.
     * @return The trimmed view, empty when it was blanks throughout.
     */
    [[nodiscard]] inline std::string_view trimmed(
        const std::string_view text) noexcept
    {
        constexpr std::string_view kBlanks{" \t"};

        const auto first = text.find_first_not_of(kBlanks);

        if (first == std::string_view::npos)
        {
            return {};
        }

        return text.substr(
            first, text.find_last_not_of(kBlanks) - first + 1);
    }

} // namespace antwika::music_editor::detail
