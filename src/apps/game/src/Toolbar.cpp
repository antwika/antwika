#include "antwika/game/Toolbar.hpp"

#include <cstddef>
#include <optional>
#include <string>

#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::game
{

    using antwika::ui::Alignment;
    using antwika::ui::ButtonState;
    using antwika::ui::Context;
    using antwika::ui::kFit;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    Frame Toolbar::describe(
        Size canvas,
        Pointer pointer,
        const Camera &camera,
        BuildTool selected,
        bool paused,
        antwika::time::Tick tick) const
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

                // Held down while paused.
                // So what the run is doing can be seen, not just read.
                ui.button(
                    std::string{pauseLabel(paused)},
                    {.id = widgets::kPauseResume,
                     .state = paused
                                  ? std::optional{ButtonState::Pressed}
                                  : std::nullopt});

                // Simulation state, read back out where it can be seen.
                ui.label("zoom " + std::to_string(camera.zoomLevel()));

                // Likewise: the tick is what a run is counted in.
                // A replay is on the same one at the same point.
                ui.label("tick " + std::to_string(tick));
            }

            {
                const auto row =
                    ui.row({.width = kFit, .cross = Alignment::Center});

                // One button per tool, in the enumeration's own order.
                // A tool added there therefore gets a button here.
                for (std::size_t index = 0; index < kBuildToolCount;
                     ++index)
                {
                    const auto tool = static_cast<BuildTool>(index);

                    // The chosen one is held down.
                    // Which it is can then be seen without hovering.
                    ui.button(
                        std::string{toolLabel(tool)},
                        {.id = widgets::toolWidget(tool),
                         .state = tool == selected
                                      ? std::optional{ButtonState::Pressed}
                                      : std::nullopt});
                }
            }
        }

        return ui.finish();
    }

} // namespace antwika::game
