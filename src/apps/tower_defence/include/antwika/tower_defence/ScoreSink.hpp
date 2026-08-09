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

    struct ScoreBarState final
    {
        std::size_t level = 1;
        std::size_t levelCount = 1;
        std::size_t wave = 1;
        std::size_t waveCount = 1;
        std::uint32_t lives = 0;
        std::uint64_t score = 0;

        std::uint64_t best = 0;

        CampaignPhase phase = CampaignPhase::Fighting;

        [[nodiscard]] bool operator==(const ScoreBarState &) const
            = default;
    };

    [[nodiscard]] ScoreBarState scoreBarStateOf(
        const Campaign &campaign, std::uint64_t best);

    [[nodiscard]] DrawList describeScoreBar(
        Size canvas,
        const Translator &translator,
        const ScoreBarState &state);

    class ScoreSink final : public ITickEventSink
    {
    public:
        ScoreSink(
            const Campaign &campaign,
            ScoreOverlay &overlay,
            const Translator &translator,
            const std::uint64_t &best);

        ScoreSink(const ScoreSink &) = delete;
        ScoreSink(ScoreSink &&) = delete;

        ScoreSink &operator=(const ScoreSink &) = delete;
        ScoreSink &operator=(ScoreSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        const Campaign &campaign;
        ScoreOverlay &overlay;
        const Translator &translator;
        const std::uint64_t &best;
    };

}
