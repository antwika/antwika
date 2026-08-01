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
        // No pointer, so nothing here can be hovered or activated.
        // Which is the whole of what "not clickable" means here.
        Context ui{canvas, scaledTheme(Theme{}, scaleForCanvas(canvas))};

        {
            const auto strip = ui.row({.width = kGrow, .height = kFit});

            // The toolbar has the left corner, so this has the right.
            // A growing spacer is how a layout puts something last.
            ui.spacer(kGrow);

            {
                const auto box =
                    ui.panel({.width = kFit, .height = kFit});

                // A rate, or the placeholder standing in for one.
                // Two calls rather than one over a conditional string.
                // The placeholder is a view of a constant.
                // So the path with nothing to report builds nothing.
                // And a path that builds nothing cannot unwind.
                // Which is the whole reason this is not a ternary.
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

} // namespace antwika::game
