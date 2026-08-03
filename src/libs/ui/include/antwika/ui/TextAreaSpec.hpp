#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief One span of a text area's characters drawn on its own
     * ground.
     *
     * Half-open, as every range here is: begin is the first
     * highlighted index and end is one past the last.
     */
    struct TextHighlight
    {
        /** @brief The first highlighted index. */
        std::size_t begin = 0;

        /** @brief One past the last highlighted index. */
        std::size_t end = 0;

        /**
         * @brief Compare two highlights.
         * @param other The highlight to compare against.
         * @return True when both ends match.
         */
        [[nodiscard]] bool operator==(const TextHighlight &other) const
            = default;
    };

    /**
     * @brief Extra rows of room held open beneath one line.
     *
     * What an application hangs a picture of its own under a line
     * with -- a live-coding editor draws a pianoroll beneath the
     * voice the band names.  The area itself draws nothing there:
     * the band is named, so where it ended up comes back through
     * Frame::rects, and the caller paints into that.
     *
     * **Whole rows rather than pixels**, so every mapping from a
     * click to a line stays the whole-line arithmetic a recorded
     * click is replayed against.  The lines beneath a band move
     * down by its rows, and scrolling, the caret and the thumb all
     * count it as part of the line it hangs under.
     */
    struct LineBand
    {
        /** @brief Which line the band hangs beneath, from zero. */
        std::size_t line = 0;

        /** @brief How many rows of room it holds open. */
        std::uint32_t rows = 0;

        /**
         * @brief What to call the band's area in Frame::rects.
         *
         * An unnamed band still holds its room; there is simply no
         * way to ask where it went.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief Compare two bands.
         * @param other The band to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const LineBand &other) const
            = default;
    };

    /**
     * @brief Where a drag over a text area began, as its caller knows.
     *
     * The track and the text are one widget to every other channel,
     * and this library retains nothing between frames -- so which of
     * the two a drag began on is the caller's to hold, exactly as
     * focus is.  Interactions::areaPress is where a press says which
     * it landed on; a caller stores that and hands it back here.
     *
     * Without it the two contaminated each other: a drag-selection
     * that strayed onto the track jumped the scroll, and a bar drag
     * that wobbled into the text started selecting from a stale
     * anchor.
     */
    enum class DragHome : std::uint8_t
    {
        /** @brief No drag is in progress over this area. */
        None = 0,

        /** @brief The press landed in the text. */
        Text,

        /** @brief The press landed on the scrollbar's track. */
        Track,
    };

    /**
     * @brief What a text area is being asked for.
     *
     * A text field over many lines, and the same bargain as one: the
     * characters are the caller's, they arrive here as a view the caller
     * owns, and an edit is reported back through Interactions::edit
     * rather than applied.
     *
     * **A line break is a character in the text like any other.**
     * So the caret is one index into the whole of it rather than a row
     * and a column, and an application storing a document plus a cursor
     * is storing everything a replay has to regenerate.
     */
    struct TextAreaSpec
    {
        /**
         * @brief What to call this area when reporting what happened.
         *
         * An area left unnamed still draws and still edits, but nothing
         * can hover it, and the edit it reports names no widget.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief How wide, defaulting to filling the room across.
         */
        Sizing width = kGrow;

        /**
         * @brief How tall, defaulting to filling the room down.
         *
         * Unlike a field, which is one line high whatever it is given:
         * an area is somewhere to write, and how much room there is to
         * write in is the container's answer rather than the text's.
         */
        Sizing height = kGrow;

        /**
         * @brief The characters the area currently holds.
         *
         * Split into lines on '\n' as it is drawn, and the caller owns
         * the buffer for as long as the Context is.
         */
        std::string_view text{};

        /**
         * @brief What to show instead while the area is empty.
         */
        std::string_view placeholder{};

        /**
         * @brief Where the caret sits, as an index into text.
         *
         * Past the end is the end, so a caller may hand back the cursor
         * of an edit it has applied without clamping it itself.
         */
        std::size_t cursor = kCaretAtEnd;

        /**
         * @brief Where the selection's other end sits.
         *
         * The characters between this and the cursor are drawn on the
         * theme's selection colour, and are what a typed character, a
         * Backspace, a Delete, a Copy or a Cut acts on instead of one
         * character.
         *
         * Absent -- which is the default -- puts it wherever the caret
         * is, so nothing is selected. Absent rather than a sentinel
         * index, because every index is a place a selection can really
         * end, the end of the text included.
         *
         * Past the end is the end, as cursor is.
         */
        std::optional<std::size_t> anchor{};

        /**
         * @brief Which line of the text is drawn at the top.
         *
         * Lines rather than pixels, so what an area shows is always
         * whole lines and a recorded click always lands on one.
         *
         * Past the last line it can usefully be is brought back, so a
         * caller may add a wheel's notches to it without knowing how
         * many lines fit. The line actually drawn at the top comes back
         * through Interactions::scrolled whenever it differs from this,
         * which is also how the caret is kept in view: an area that
         * reports an edit this frame scrolls to wherever its caret
         * ended up.
         */
        std::size_t scroll = 0;

        /**
         * @brief Spans drawn on the theme's highlight ground.
         *
         * What a caller says is *sounding, matching, or otherwise
         * alive* right now -- a live-coding editor lights the notes
         * being played with it.  Purely a change of ground: no
         * highlight moves the caret, joins a selection or survives
         * the frame, and the selection wins where the two overlap.
         *
         * The caller owns them for as long as the Context is, and
         * ends past the text are clamped as the caret's is.
         */
        std::span<const TextHighlight> highlights{};

        /**
         * @brief The bands of extra room held open beneath lines.
         *
         * The caller owns them for as long as the Context is, as it
         * owns the text.  A band naming a line the text does not have
         * holds nothing, one of no rows holds nothing, and two bands
         * naming one line stack in declaration order.  A band whose
         * line is scrolled off the top is off with it, and is then
         * absent from Frame::rects exactly as a collapsed widget is.
         */
        std::span<const LineBand> bands{};

        /**
         * @brief Whether to draw a bar down the right-hand edge saying
         * how much of the text is showing.
         *
         * It takes its width out of the room the text has, and a press
         * or a drag anywhere on it scrolls, reported through
         * Interactions::scrolled. An area without one still scrolls;
         * there is simply nothing to grab.
         */
        bool scrollbar = false;

        /**
         * @brief Whether this is the area the typing belongs to.
         *
         * The caret is drawn only for a focused area, and only a focused
         * area reports an edit.
         */
        bool focused = false;

        /**
         * @brief Where the drag in progress began, if one is.
         *
         * Handed back by the caller from Interactions::areaPress, and
         * what scopes a held drag: a drag that began in the text never
         * reaches the track, and one that began on the track never
         * selects.  A fresh press needs no scoping; it lands where it
         * lands.
         */
        DragHome dragging = DragHome::None;
    };

} // namespace antwika::ui
