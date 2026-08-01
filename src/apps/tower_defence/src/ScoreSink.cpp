#include "antwika/tower_defence/ScoreSink.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include <antwika/engine/Events.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::tower_defence
{

    using antwika::i18n::MessageId;
    using antwika::ui::Alignment;
    using antwika::ui::Context;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        std::string oneNumber(
            const Translator &translator,
            const MessageId id,
            const std::uint64_t value)
        {
            const std::string text = std::to_string(value);
            const std::array<std::string_view, 1> args{text};
            return translator.formatted(id, args);
        }

        std::string twoNumbers(
            const Translator &translator,
            const MessageId id,
            const std::uint64_t first,
            const std::uint64_t second)
        {
            const std::string one = std::to_string(first);
            const std::string two = std::to_string(second);
            const std::array<std::string_view, 2> args{one, two};
            return translator.formatted(id, args);
        }

        // The middle of the bar says what there is to do next.
        // While a level is being fought that is which wave is out.
        // Once the campaign is over it is which way it ended.
        // Which is the one thing worth the room by then.
        std::string phaseText(
            const Translator &translator, const ScoreBarState &state)
        {
            if (state.phase == CampaignPhase::Won)
            {
                return translator.text(MessageId::TowerDefenceCleared);
            }
            if (state.phase == CampaignPhase::Lost)
            {
                return translator.text(MessageId::TowerDefenceOverrun);
            }

            return twoNumbers(
                translator,
                MessageId::TowerDefenceWave,
                state.wave,
                state.waveCount);
        }
    } // namespace

    ScoreBarState scoreBarStateOf(
        const Campaign &campaign, const std::uint64_t best)
    {
        const Battle &battle = campaign.battle();
        const std::size_t levels = campaign.levelCount();
        const std::size_t waves = battle.waveCount();

        // Both counts are clamped rather than left to run past.
        // A finished campaign steps its level index past the last one.
        // A level whose waves are all out does the same with its own.
        return ScoreBarState{
            .level = std::min(campaign.levelIndex() + 1, levels),
            .levelCount = levels,
            .wave = std::min(battle.wavesReleased() + 1, waves),
            .waveCount = waves,
            .lives = campaign.lives(),
            .score = campaign.score(),
            .best = best,
            .phase = campaign.phase()};
    }

    DrawList describeScoreBar(
        const Size canvas,
        const Translator &translator,
        const ScoreBarState &state)
    {
        // No pointer is handed in, deliberately.
        // Nothing on this bar is clickable, so nothing is hit-tested.
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas))};

        {
            const auto bar = ui.panel({.width = kGrow, .height = kFit});

            {
                const auto row =
                    ui.row({.width = kGrow, .cross = Alignment::Center});

                ui.label(twoNumbers(
                    translator,
                    MessageId::TowerDefenceLevel,
                    state.level,
                    state.levelCount));
                ui.spacer(kGrow);
                ui.label(phaseText(translator, state));
                ui.spacer(kGrow);
                ui.label(oneNumber(
                    translator,
                    MessageId::TowerDefenceLives,
                    state.lives));
                ui.spacer(kGrow);
                ui.label(oneNumber(
                    translator,
                    MessageId::TowerDefenceScore,
                    state.score));
                ui.spacer(kGrow);
                ui.label(oneNumber(
                    translator,
                    MessageId::TowerDefenceBest,
                    state.best));
            }
        }

        return ui.finish().commands;
    }

    ScoreSink::ScoreSink(
        const Campaign &campaign,
        ScoreOverlay &overlay,
        const Translator &translator,
        const std::uint64_t best)
        : campaign(campaign),
          overlay(overlay),
          translator(translator),
          best(best)
    {
    }

    void ScoreSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        overlay.set(describeScoreBar(
            overlay.canvas(),
            translator,
            scoreBarStateOf(campaign, best)));
    }

} // namespace antwika::tower_defence
