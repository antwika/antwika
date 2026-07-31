#include "antwika/tower_defence/ScoreSink.hpp"

#include <string>

#include <antwika/engine/Events.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::tower_defence
{

    using antwika::ui::Alignment;
    using antwika::ui::Context;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    DrawList describeScoreBar(
        const Size canvas,
        const std::uint64_t score,
        const std::uint32_t leaks)
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

                ui.label("SCORE " + std::to_string(score));
                ui.spacer(kGrow);
                ui.label("LEAKED " + std::to_string(leaks));
            }
        }

        return ui.finish().commands;
    }

    ScoreSink::ScoreSink(const Battle &battle, ScoreOverlay &overlay)
        : battle(battle), overlay(overlay)
    {
    }

    void ScoreSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        overlay.set(describeScoreBar(
            overlay.canvas(), battle.score(), battle.leaks()));
    }

} // namespace antwika::tower_defence
