#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/notation/NumberWords.hpp>
#include <antwika/pattern/Pattern.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    using antwika::pattern::Pattern;

    /**
     * @brief Get the document an empty editor opens with.
     *
     * Something that plays, because an editor that opens silent gives a
     * newcomer nothing to change.
     *
     * @return The document, written in the syntax Score reads.
     */
    [[nodiscard]] std::string openingSource();

    /**
     * @brief What one line of the document was refused for.
     */
    struct Problem
    {
        /** @brief Which line of the document, counting from one. */
        std::size_t line = 0;

        /** @brief What was wrong with it. */
        std::string message{};

        /**
         * @brief Compare two problems.
         * @param other The problem to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(const Problem &other) const
            = default;
    };

    /**
     * @brief What the document currently plays.
     *
     * **This is where live editing becomes live playback.**
     * Nothing tells it to reload: it is handed the whole document every
     * tick and works out for itself what changed.
     *
     * The document is code rather than one box per voice:
     *
     * @code
     * // a comment, and a blank line, are passed over
     * $: bass 0 ~ 0 [~ 3]
     * $: drum 0(3,8)
     * @endcode
     *
     * A line opens with `$:` and names one of the four voices; what
     * follows it is the mini-notation that voice plays. **A voice is
     * named rather than counted**, so writing a line above another does
     * not move that other one to a different instrument -- which is the
     * one thing a positional syntax gets wrong exactly while somebody
     * is typing into the middle of a score.
     *
     * **A line that does not parse keeps playing whatever it last
     * did.**
     * That is the decision the whole feel of the editor rests on: half
     * a bracket is typed on the way to a whole one, and an editor that
     * fell silent at every intermediate keystroke would be unusable.
     * The refusal is reported through problems() instead, and the
     * moment the line reads again the new pattern takes over.
     *
     * A voice no line names falls silent, since deleting a line is how
     * an instrument is taken out.
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
         * @brief Re-read the document, if it has changed.
         *
         * Costs nothing on a tick where nothing was typed, and parses
         * again only the voice lines whose notation actually differs.
         *
         * @param source What the editor now holds.
         */
        void read(const std::string &source);

        /**
         * @brief Get what one voice is playing.
         * @param track Which track.
         * @return Its pattern, which is the last one that parsed.
         */
        [[nodiscard]] const Pattern &playing(std::size_t track) const;

        /**
         * @brief Get every line the document was refused for.
         * @return The problems, in ascending line order.
         */
        [[nodiscard]] const std::vector<Problem> &
            problems() const noexcept;

        /**
         * @brief Get whether any line is currently refused.
         * @return True when at least one problem stands.
         */
        [[nodiscard]] bool hasError() const noexcept;

        /**
         * @brief Get how many voice lines have been parsed since the
         * start.
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

        void readLine(std::string_view line, std::size_t number);

        void play(
            std::size_t track,
            std::string_view notation,
            std::size_t number);

        void refuse(std::size_t number, std::string message);

        notation::NumberWords words;
        std::vector<Track> tracks;
        std::vector<Problem> refusals;

        std::string document;
        std::array<bool, kTrackCount> claimed{};

        bool everRead = false;
        std::size_t parsed = 0;
    };

} // namespace antwika::music_editor
