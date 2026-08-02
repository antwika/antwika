#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include <antwika/notation/NumberWords.hpp>
#include <antwika/pattern/Pattern.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    using antwika::pattern::Pattern;

    /**
     * @brief What every line currently parses into.
     *
     * **This is where live editing becomes live playback.**
     * Nothing tells it to reload: it is handed the lines every tick and
     * works out for itself which of them changed.
     *
     * **A line that does not parse keeps playing whatever it last
     * did.**
     * That is the decision the whole feel of the editor rests on: half
     * a bracket is typed on the way to a whole one, and an editor that
     * fell silent at every intermediate keystroke would be unusable.
     * The error is reported beside the line instead, and the moment the
     * line parses again the new pattern takes over.
     */
    class Score final
    {
    public:
        /**
         * @brief Build a score playing nothing at all.
         */
        Score();

        Score(const Score &) = delete;
        Score(Score &&) = delete;

        Score &operator=(const Score &) = delete;
        Score &operator=(Score &&) = delete;

        /**
         * @brief Re-read whichever lines have changed.
         *
         * Cheap on a tick where nothing was typed, because a line whose
         * characters are the ones last read is not parsed again.
         *
         * @param lines What the editor now holds.
         */
        void update(const std::array<std::string, kTrackCount> &lines);

        /**
         * @brief Get what one track is playing.
         * @param track Which track.
         * @return Its pattern, which is the last one that parsed.
         */
        [[nodiscard]] const Pattern &playing(std::size_t track) const;

        /**
         * @brief Get why one track's line was refused.
         * @param track Which track.
         * @return The message, or empty when the line reads cleanly.
         */
        [[nodiscard]] const std::string &error(
            std::size_t track) const noexcept;

        /**
         * @brief Get whether any line is currently refused.
         * @return True when at least one line has an error.
         */
        [[nodiscard]] bool hasError() const noexcept;

        /**
         * @brief Get how many lines have been re-read since the start.
         *
         * For a test and for the status line, so that "the edit took"
         * is observable without listening to it.
         *
         * @return The count.
         */
        [[nodiscard]] std::size_t reparses() const noexcept;

    private:
        struct Track
        {
            std::string source;
            std::string failure;
            Pattern playing;
        };

        notation::NumberWords words;
        std::vector<Track> tracks;

        std::size_t parsed = 0;
    };

} // namespace antwika::music_editor
