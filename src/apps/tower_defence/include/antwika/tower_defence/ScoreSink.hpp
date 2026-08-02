#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/Messages.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief Everything the score bar says, as numbers.
     *
     * A value between the campaign and the words, so what the bar shows
     * can be asserted without a translator and the wording can change
     * without touching the arithmetic.
     * The level and the wave are counted from one here, since that is
     * how they are read out.
     */
    struct ScoreBarState
    {
        std::size_t level = 1;
        std::size_t levelCount = 1;
        std::size_t wave = 1;
        std::size_t waveCount = 1;
        std::uint32_t lives = 0;
        std::uint64_t score = 0;

        /**
         * @brief The best score any earlier run reached.
         *
         * Run configuration rather than simulation state: it is read
         * from the high-score file once, before the loop starts, and
         * held for the run. Nothing is hit-tested against this bar, so
         * a run started against a different best is still the same
         * simulation -- and a replay is handed no store at all, so it
         * shows nothing there. See the wiki page for why that boundary
         * is where it is.
         */
        std::uint64_t best = 0;

        CampaignPhase phase = CampaignPhase::Fighting;

        [[nodiscard]] bool operator==(const ScoreBarState &) const
            = default;
    };

    /**
     * @brief Read the bar's numbers off a campaign.
     * @param campaign What is being fought.
     * @param best The best score of any earlier run.
     * @return What the bar should say.
     */
    [[nodiscard]] ScoreBarState scoreBarStateOf(
        const Campaign &campaign, std::uint64_t best);

    /**
     * @brief Lay the score bar out and say what it draws.
     *
     * A free function so the picture can be asserted with EXPECT_EQ and
     * no window: what antwika::ui hands back is a list of values.
     *
     * One row, whatever the state, because the strip reserved above the
     * grid is one row tall -- see scoreBarHeight(). A second row would
     * leave the bar covering cells a click could still reach.
     *
     * @param canvas The area the bar is laid out into.
     * @param translator Words every label; injected, never reached for.
     * @param state What to say.
     * @return The bar's drawing commands, in paint order.
     */
    [[nodiscard]] DrawList describeScoreBar(
        Size canvas,
        const Translator &translator,
        const ScoreBarState &state);

    /**
     * @brief Puts the campaign's standing into the overlay, once a tick.
     *
     * **Describing the UI happens here, downstream of the recorder, and
     * never in a renderer.**
     * The bar is a pure function of the campaign's state and one number
     * fixed for the run, so a replay that regenerates the state
     * regenerates the bar; this application therefore defines no event
     * for it and no `ui.*` name exists.
     *
     * Registered after CampaignSink, so the numbers it shows are the
     * ones the tick ended with rather than the ones it started with.
     */
    class ScoreSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over what it reads and writes.
         * @param campaign Read for the numbers. Must outlive this sink.
         * @param overlay Written every tick. Must outlive this sink.
         * @param translator Words the labels. Must outlive this sink.
         * @param best The best score of any earlier run, fixed for this
         * one.
         */
        ScoreSink(
            const Campaign &campaign,
            ScoreOverlay &overlay,
            const Translator &translator,
            std::uint64_t best);

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
        const Campaign &campaign;
        ScoreOverlay &overlay;
        const Translator &translator;
        std::uint64_t best;
    };

} // namespace antwika::tower_defence
