#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    /**
     * @brief One voice line, read.
     */
    struct VoiceChain
    {
        /** @brief The sound, after every call has had its say. */
        TrackPreset preset{};

        /** @brief The mini-notation the `n(...)` call carried. */
        std::string notation{};

        /**
         * @brief Where that notation's first character sits in the
         * chain.
         *
         * What maps a span inside the notation back onto the document
         * a highlight is drawn over; see Score::spanIn().
         */
        std::size_t notationAt = 0;

        /**
         * @brief Whether the line asked for a pianoroll beneath it.
         *
         * A display request rather than a sound: the preset is not
         * touched, and Score::pianorolls() is where it comes out.
         */
        bool pianoroll = false;

        /**
         * @brief Whether the line asked for a waveform beneath it.
         *
         * A display request on pianoroll's terms, coming out through
         * Score::waveforms().
         */
        bool waveform = false;

        /**
         * @brief Compare two chains.
         * @param other The chain to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(const VoiceChain &other) const
            = default;
    };

    /**
     * @brief Read a voice line into a sound and a pattern.
     *
     * A chain of calls joined by dots, optionally opening with the name
     * of a preset to start from:
     *
     * @code
     * drum.n("0(3,8)").gain(.25).pan(-.4)
     * n("<12 7> ~ 10").s(square).base(440).lpf(3000).o(1)
     * @endcode
     *
     * **A call changes a copy**, which is the whole point of naming a
     * preset rather than being one: two lines opening `drum.` are two
     * voices that sound together and can differ in every other respect.
     *
     * The dots that separate calls are found at depth zero and outside
     * quotes, so a number like `.25` and a notation like `"0 . 3"` are
     * not mistaken for separators.
     *
     * Every call takes exactly one argument, except `pianoroll` and
     * `waveform`, which take none: each asks for a picture rather
     * than a sound, and there is nothing about a picture to say.
     * `n` takes a quoted string; every other takes a number, except
     * `s`, which takes a shape's name.
     *
     * @param chain What followed the `$:` on the line.
     * @return The sound and the notation it plays.
     * @throws ScoreError If a call, an argument or a preset name is not
     * one this editor has, or if the chain carries no `n(...)`.
     */
    [[nodiscard]] VoiceChain parseVoiceChain(std::string_view chain);

    /**
     * @brief Get every control a chain may name.
     *
     * For the message a refusal ends with, so that the list a person
     * reads cannot drift from the list the parser accepts.
     *
     * @return The names, space-separated, outliving every caller.
     */
    [[nodiscard]] std::string_view voiceControls() noexcept;

} // namespace antwika::music_editor
