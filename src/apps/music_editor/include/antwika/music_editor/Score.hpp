#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/notation/NumberWords.hpp>
#include <antwika/pattern/Pattern.hpp>
#include <antwika/pattern/Patterns.hpp>

#include "antwika/music_editor/TrackPreset.hpp"
#include "antwika/music_editor/VoiceChain.hpp"

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
     * @brief One voice: a sound, and what it is playing.
     */
    struct Voice
    {
        TrackPreset preset{};

        // Silence rather than nothing.
        // A pattern has no empty value to hold instead.
        Pattern playing = pattern::silence();
    };

    /**
     * @brief What the document currently plays.
     *
     * **This is where live editing becomes live playback.**
     * Nothing tells it to reload: it is handed the whole document every
     * tick and works out for itself what changed.
     *
     * The document is code.
     * A line opening with `//`, or holding nothing, is passed over; a
     * voice line opens with `$:` and carries a chain of calls:
     *
     * @code
     * // two drums, sounding together, out of one preset
     * $: drum.n("0(3,8)")
     * $: drum.n("~ ~ [0 0] ~").gain(.2).pan(.5).hpf(4000)
     * $: bass.n("0 ~ 0 [~ 3]").o(-1)
     * @endcode
     *
     * **A line is a voice, and nothing is limited to one of a kind.**
     * A preset is a starting point that the chain after it changes a
     * copy of, so two lines opening `drum.` are two voices that sound
     * at once and can differ in every other respect. What a voice is
     * called is nothing: the line *is* the identity.
     *
     * **A line that does not read keeps playing whatever it last
     * did.**
     * That is the decision the whole feel of the editor rests on: half
     * a bracket is typed on the way to a whole one, and an editor that
     * fell silent at every intermediate keystroke would be unusable.
     * The refusal is reported through problems() instead, naming the
     * line it came from, and the moment the line reads again the new
     * voice takes over.
     *
     * Deleting a line takes its voice out, since a voice is a line.
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
         * Costs nothing on a tick where nothing was typed, and reads
         * again only the lines whose text actually differs.
         *
         * @param source What the editor now holds.
         */
        void read(const std::string &source);

        /**
         * @brief Get every voice the document is sounding.
         * @return The voices, in the order their lines appear.
         */
        [[nodiscard]] const std::vector<Voice> &voices() const noexcept;

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
         * @brief Get how many voice lines have been read since the
         * start.
         *
         * For a test and for the status line, so that "the edit took"
         * is observable without listening to it.
         *
         * @return The count.
         */
        [[nodiscard]] std::size_t reparses() const noexcept;

    private:
        // What one voice line came to, kept for two reasons.
        // The next read tells an unchanged line from an edited one.
        // And a line that stops reading keeps its last voice.
        struct Line
        {
            std::string chain;
            std::string failure;
            bool sounding = false;
            Voice voice;
        };

        void readLine(std::string_view line, std::size_t number);

        void play(std::string_view chain, std::size_t number);

        void refuse(std::size_t number, std::string message);

        notation::NumberWords words;

        std::vector<Line> lines;
        std::vector<Voice> sounding;
        std::vector<Problem> refusals;

        std::string document;

        // How many voice lines this read has taken.
        // Which is also where the next one is kept.
        std::size_t seen = 0;

        std::size_t parsed = 0;

        bool everRead = false;
    };

} // namespace antwika::music_editor
