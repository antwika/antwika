#pragma once

#include <cstdint>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief Lay the score bar out and say what it draws.
     *
     * A free function so the picture can be asserted with EXPECT_EQ and
     * no window: what antwika::ui hands back is a list of values.
     *
     * @param canvas The area the bar is laid out into.
     * @param score The running total to show.
     * @param leaks How many mobs have reached the end.
     * @return The bar's drawing commands, in paint order.
     */
    [[nodiscard]] DrawList describeScoreBar(
        Size canvas, std::uint64_t score, std::uint32_t leaks);

    /**
     * @brief Puts the running score into the overlay, once per tick.
     *
     * **Describing the UI happens here, downstream of the recorder, and
     * never in a renderer.**
     * The bar is a pure function of the battle's state, so a replay that
     * regenerates the state regenerates the bar; this application
     * therefore defines no event for it and no `ui.*` name exists.
     *
     * Registered after BattleSink, so the number it shows is the one the
     * tick ended with rather than the one it started with.
     */
    class ScoreSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over what it reads and writes.
         * @param battle Read for the score. Must outlive this sink.
         * @param overlay Written every tick. Must outlive this sink.
         */
        ScoreSink(const Battle &battle, ScoreOverlay &overlay);

        ScoreSink(const ScoreSink &) = delete;
        ScoreSink(ScoreSink &&) = delete;

        ScoreSink &operator=(const ScoreSink &) = delete;
        ScoreSink &operator=(ScoreSink &&) = delete;

        /**
         * @brief Describe the bar if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        const Battle &battle;
        ScoreOverlay &overlay;
    };

} // namespace antwika::tower_defence
