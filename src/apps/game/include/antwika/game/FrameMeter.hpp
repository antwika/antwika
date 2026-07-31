#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <antwika/time/IClock.hpp>

namespace antwika::game
{

    using antwika::time::IClock;

    /**
     * @brief How long a stretch of drawing one rate is measured over.
     *
     * One second, so the number on screen is "frames per second" rather
     * than a figure needing a caption. A shorter window would jitter and
     * a longer one would lag a stutter somebody is trying to see.
     */
    inline constexpr std::chrono::milliseconds kFpsWindow{1000};

    /**
     * @brief Counts drawn frames against a wall clock.
     *
     * **This is render-side state and may never be read by anything a
     * replay reproduces.** A wall clock says how fast the machine is,
     * and a run that read one would compute a different result on a
     * slower one -- which is the exact thing the replay system exists to
     * prevent. So nothing here reaches a sink, a system, a snapshot,
     * GameSummary or a save: the one reader is RenderSystem, and what it
     * does with the number is draw it.
     *
     * That is the same safety condition input::PointerHintChannel is
     * held to, arrived at from the other side: the hint is a value a
     * replay does not reproduce, and this is a value a replay must not
     * see at all.
     *
     * The clock is injected rather than read from
     * std::chrono::system_clock here, so a test hands it a fake and
     * asserts an exact number instead of a plausible one.
     *
     * The rate is frames counted since the window opened divided by how
     * long it has been open, so it counts the gaps between frames rather
     * than the frames themselves -- twenty-five frames spanning a second
     * is twenty-five per second, not twenty-six.
     */
    class FrameMeter final
    {
    public:
        /**
         * @brief Construct the meter over the clock it measures against.
         * @param clock Read once per recorded frame. Must outlive this
         * meter.
         */
        explicit FrameMeter(const IClock &clock) noexcept;

        FrameMeter(const FrameMeter &) = delete;
        FrameMeter(FrameMeter &&) = delete;

        FrameMeter &operator=(const FrameMeter &) = delete;
        FrameMeter &operator=(FrameMeter &&) = delete;

        /**
         * @brief Note that one frame has just been drawn.
         *
         * The first one opens the window rather than being counted into
         * it, since a rate needs two frames and a gap between them.
         */
        void record();

        /**
         * @brief Get the rate the last full window came to.
         * @return Frames per second, and zero until a whole window has
         * gone by -- a run that has drawn for less than a second has no
         * measurement to report, and reporting a guess would be worse
         * than reporting nothing.
         */
        [[nodiscard]] std::uint32_t perSecond() const noexcept;

    private:
        using TimePoint =
            std::chrono::time_point<std::chrono::system_clock>;

        const IClock &clock;
        std::optional<TimePoint> windowStart{};
        std::uint32_t counted = 0;
        std::uint32_t rate = 0;
    };

} // namespace antwika::game
