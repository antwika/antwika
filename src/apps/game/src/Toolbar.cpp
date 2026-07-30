#include "antwika/game/Toolbar.hpp"

#include <string>

#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::game
{

    using antwika::ui::Alignment;
    using antwika::ui::Context;
    using antwika::ui::kFit;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    Frame Toolbar::describe(
        Size canvas, Pointer pointer, const Camera &camera) const
    {
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            const auto bar = ui.panel({.width = kFit, .height = kFit});

            {
                const auto row =
                    ui.row({.width = kFit, .cross = Alignment::Center});

                ui.button("zoom out", {.id = widgets::kZoomOut});
                ui.button("zoom in", {.id = widgets::kZoomIn});
                ui.button("reset view", {.id = widgets::kResetView});

                // Simulation state, read back out where it can be seen.
                ui.label("zoom " + std::to_string(camera.zoomLevel()));
            }
        }

        return ui.finish();
    }

} // namespace antwika::game
