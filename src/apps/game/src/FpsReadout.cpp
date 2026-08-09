#include "antwika/game/FpsReadout.hpp"

#include <optional>
#include <string>

#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::game
{

    using antwika::ui::Context;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    DrawList describeFps(
        Size canvas, std::optional<std::uint32_t> framesPerSecond)
    {
        Context ui{canvas, scaledTheme(Theme{}, scaleForCanvas(canvas))};

        {
            const auto strip = ui.row({.width = kGrow, .height = kFit});

            ui.spacer(kGrow);

            {
                const auto box =
                    ui.panel({.width = kFit, .height = kFit});

                if (framesPerSecond.has_value())
                {
                    ui.label("fps " + std::to_string(*framesPerSecond));
                }
                else
                {
                    ui.label(kNoRateReadout);
                }
            }
        }

        return ui.finish().commands;
    }

}
