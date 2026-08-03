#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/pattern/Pattern.hpp>
#include <antwika/pattern/Patterns.hpp>

#include "antwika/music_editor/NoteWords.hpp"
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
     * @brief A run of document characters, for a highlight to sit on.
     *
     * Half-open, exactly as ui::TextHighlight is.
     */
    struct DocumentSpan
    {
        /** @brief The first character's index into the document. */
        std::size_t begin = 0;

        /** @brief One past the last character's index. */
        std::size_t end = 0;

        /**
         * @brief Compare two spans.
         * @param other The span to compare against.
         * @return True when both ends match.
         */
        [[nodiscard]] bool operator==(const DocumentSpan &other) const
            = default;
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
     * **A chain may run down as many lines as it likes**, and a line
     * opening with a dot is the one below carrying on:
     *
     * @code
     * $: bass.n("0 ~ 0 [~ 3]")
     *      .o(-1)
     *      .lpf(900).res(.6)
     * @endcode
     *
     * That is one voice, refused or sounded as one, and named by the
     * line its `$:` is on. The dot is the join rather than something at
     * the end of the line above it, so a chain being written stays
     * legible while it is half typed: every line of it reads as a call.
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
         * @brief Map a note's span back onto the document.
         *
         * The other end of what NoteWords wrote into every event: a
         * note carries where its word sat in its n("...") string, and
         * this walks that offset back through the chain the line was
         * gathered from and onto the characters as the document now
         * holds them -- so a highlight lands on what is on screen,
         * however the lines above it have moved since the note was
         * decided.
         *
         * Nothing when the voice is gone, the offset runs past what
         * the line now reads, or the word straddles a gathered line's
         * edge it no longer has; a highlight that cannot land anywhere
         * honest is dropped rather than guessed at.
         *
         * @param voice An index into voices().
         * @param begin Where the word starts in the voice's notation.
         * @param length How many characters it runs for.
         * @return Where that word sits in the document, if it still
         * does.
         */
        [[nodiscard]] std::optional<DocumentSpan> spanIn(
            std::size_t voice,
            std::size_t begin,
            std::size_t length) const noexcept;

        /**
         * @brief Get the chain text a sounding voice was read from.
         *
         * The one identity a voice keeps across edits: an index into
         * voices() outlives an insertion above it, and then names a
         * different line.
         * A note lit under one chain is dropped rather than guessed
         * at when the text at its index changes -- though two lines
         * holding identical chains are indistinguishable here, and
         * light alike.
         *
         * @param voice An index into voices().
         * @return Its chain's text, empty for an index that is out of
         * range.  Borrowed until the next read().
         */
        [[nodiscard]] std::string_view chainOf(
            std::size_t voice) const noexcept;

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
        // One stretch of a gathered chain, and where it came from.
        // A chain is lines joined end to end; this is one line's part.
        struct Segment
        {
            std::size_t chainBegin = 0;
            std::size_t documentBegin = 0;
            std::size_t length = 0;
        };

        // What one voice line came to, kept for two reasons.
        // The next read tells an unchanged line from an edited one.
        // And a line that stops reading keeps its last voice.
        struct Line
        {
            std::string chain;
            std::string failure;

            // Where each stretch of the chain sits in the document.
            // Refreshed on every read, since lines above it move it.
            std::vector<Segment> segments;

            // Where the notation's characters begin in the chain.
            // Set when the chain parses, like the voice beside it.
            std::size_t notationAt = 0;

            // Whether this line has ever been read at all.
            // A line kept but never read holds an empty chain.
            // Which is exactly what a bare `$:` yields.
            // Without this the one would be taken for the other.
            bool ever = false;

            bool sounding = false;
            Voice voice;
        };

        // One voice being gathered, which may take several lines.
        // Empty text with no line means nothing is being gathered.
        struct Gathered
        {
            std::string chain;

            // Where each appended stretch sits in the document.
            std::vector<Segment> segments;

            // The line the $: was on, counting from one.
            // Zero for a voice that has not been opened.
            std::size_t opened = 0;
        };

        void readLine(
            std::string_view line,
            std::size_t number,
            std::size_t lineBegin,
            Gathered &gathering);

        void finish(Gathered &gathering);

        void play(const Gathered &gathering);

        void refuse(std::size_t number, std::string message);

        NoteWords words;

        std::vector<Line> lines;
        std::vector<Voice> sounding;

        // Which line each sounding voice came from.
        // What spanIn() follows back from a voices() index.
        std::vector<std::size_t> soundingLines;

        std::vector<Problem> refusals;

        std::string document;

        // How many voice lines this read has taken.
        // Which is also where the next one is kept.
        std::size_t seen = 0;

        std::size_t parsed = 0;

        bool everRead = false;
    };

} // namespace antwika::music_editor
