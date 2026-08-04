#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/TextFieldSpec.hpp>

namespace antwika::console
{

    using antwika::gfx::Point;
    using antwika::gfx::Size;

    /**
     * @brief How many ticks the console takes to slide in or out.
     *
     * Ticks rather than frames or milliseconds, because whether the
     * console is open decides what a key press means -- so how far
     * along the slide is, is simulation state, and simulation state
     * advances only with the tick.
     */
    inline constexpr std::uint32_t kConsoleAnimTicks = 8;

    /**
     * @brief Get how tall the console stands part way through its slide.
     *
     * The eased height, in canvas pixels: zero at rest, half the canvas
     * fully open, and a tween::Easing::CubicOut curve between the two.
     * Exact rational arithmetic throughout, so the same step is the
     * same pixel on every toolchain -- see antwika::tween for why the
     * curve list stops at polynomials.
     *
     * @param steps How many ticks of the slide have run, at most
     * kConsoleAnimTicks.
     * @param canvas The canvas the console drops down over.
     * @return The height in pixels, never past half the canvas.
     */
    [[nodiscard]] std::uint32_t consoleHeightAt(
        std::uint32_t steps, Size canvas);

    /**
     * @brief Everything the debug console remembers between frames.
     *
     * **All of it is simulation state**, in exactly the sense the
     * camera and the selected tool are: whether the console is open
     * decides whether a key press types or plays, and how far open it
     * is decides whether the input field reads at all.
     * It is therefore folded by the tick path, downstream of the
     * recorder, and never persisted as itself: a replay regenerates
     * every keystroke's meaning from the recorded input, and no
     * `console.*` event exists for any of it.
     *
     * The history is deliberately session-only.
     * Nothing writes it to disk on the way out, so a run's commands
     * are forgotten when the process ends.
     */
    class ConsoleState final
    {
    public:
        /**
         * @brief Construct the state a run opens with: closed, empty,
         * with nothing typed and nothing remembered.
         */
        ConsoleState() noexcept = default;

        ConsoleState(const ConsoleState &) = delete;
        ConsoleState(ConsoleState &&) = delete;

        ConsoleState &operator=(const ConsoleState &) = delete;
        ConsoleState &operator=(ConsoleState &&) = delete;

        /**
         * @brief Ask for the opposite of what the slide is heading for.
         *
         * Pressed mid-slide, the console turns back from wherever it
         * is rather than snapping to either end.
         */
        void toggle() noexcept;

        /**
         * @brief Advance the slide one tick toward where it is heading.
         */
        void advance() noexcept;

        /**
         * @brief Check whether any of the console is on screen.
         * @return True from the first tick of the slide in to the last
         * tick of the slide out.
         */
        [[nodiscard]] bool visible() const noexcept;

        /**
         * @brief Check whether the input field reads.
         *
         * Only a console standing fully open takes typing, which is
         * what makes every keystroke's meaning a function of state a
         * replay reaches again.
         *
         * @return True while fully open and not on its way out.
         */
        [[nodiscard]] bool acceptsText() const noexcept;

        /**
         * @brief Get how many ticks of the slide have run.
         * @return The count, at most kConsoleAnimTicks.
         */
        [[nodiscard]] std::uint32_t steps() const noexcept;

        /**
         * @brief Take this tick's eased height.
         * @param pixels The height consoleHeightAt() answered.
         */
        void setHeight(std::uint32_t pixels) noexcept;

        /**
         * @brief Get the height the last tick left the console at.
         * @return The height in canvas pixels, zero while closed.
         */
        [[nodiscard]] std::uint32_t height() const noexcept;

        /**
         * @brief Check whether the console stands over a canvas pixel.
         *
         * What ConsoleGatedSink keeps a press or a scroll off the city
         * with, resolved against the height the tick path wrote -- a
         * function of recorded input alone, so a replay answers it
         * identically.
         *
         * @param at The canvas pixel to ask about.
         * @return True while the console is out and the pixel is above
         * its bottom edge.
         */
        [[nodiscard]] bool covers(Point at) const noexcept;

        /**
         * @brief Get what has been typed into the input field.
         * @return The characters, empty until something is typed.
         */
        [[nodiscard]] const std::string &line() const noexcept;

        /**
         * @brief Get where the input field's caret sits.
         * @return An index into line().
         */
        [[nodiscard]] std::size_t caret() const noexcept;

        /**
         * @brief Take an edit the UI reported.
         * @param text What the field's characters became.
         * @param cursor Where the caret ended up.
         */
        void setLine(std::string text, std::size_t cursor);

        /**
         * @brief Take the typed line out, leaving the field empty.
         * @return What the field held.
         */
        [[nodiscard]] std::string takeLine();

        /**
         * @brief Get every line the console has said or been told.
         * @return The lines, oldest first.
         */
        [[nodiscard]] const std::vector<std::string> &
        history() const noexcept;

        /**
         * @brief Append a line under everything already listed.
         * @param entry The line to remember.
         */
        void pushHistory(std::string entry);

        /**
         * @brief Replace the history with a loaded run's.
         *
         * What load_state applies: the dump carries the console the
         * state was taken from, and coming back to that instant means
         * reading what it read.
         *
         * @param lines The lines to remember instead, oldest first.
         */
        void replaceHistory(std::vector<std::string> lines);

    private:
        bool wanted = false;
        std::uint32_t along = 0;
        std::uint32_t tall = 0;
        std::string typed;
        std::size_t cursor = antwika::ui::kCaretAtEnd;
        std::vector<std::string> lines;
    };

} // namespace antwika::console
