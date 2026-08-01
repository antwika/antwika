#include "antwika/game/Toolbar.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/i18n/MessageId.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::game
{

    using antwika::i18n::MessageId;
    using antwika::ui::Alignment;
    using antwika::ui::ButtonState;
    using antwika::ui::Context;
    using antwika::ui::kFit;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    Toolbar::Toolbar(const Translator &translator) : translator(translator)
    {
    }

    Frame Toolbar::describe(
        Size canvas,
        Pointer pointer,
        const Camera &camera,
        std::optional<BuildTool> selected,
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

                ui.button(
                    translator.text(MessageId::ToolbarZoomOut),
                    {.id = widgets::kZoomOut});
                ui.button(
                    translator.text(MessageId::ToolbarZoomIn),
                    {.id = widgets::kZoomIn});
                ui.button(
                    translator.text(MessageId::ToolbarResetView),
                    {.id = widgets::kResetView});

                // Held down while paused.
                // So what the run is doing can be seen, not just read.
                ui.button(
                    translator.text(pauseLabel(paused)),
                    {.id = widgets::kPauseResume,
                     .state = paused
                                  ? std::optional{ButtonState::Pressed}
                                  : std::nullopt});

                // Simulation state, read back out where it can be seen.
                const auto zoom = std::to_string(camera.zoomLevel());
                const std::array<std::string_view, 1> zoomArgs{zoom};
                ui.label(
                    translator.formatted(
                        MessageId::ToolbarZoomLevel, zoomArgs));

                // Likewise: the tick is what a run is counted in.
                // A replay is on the same one at the same point.
                const auto counted = std::to_string(tick);
                const std::array<std::string_view, 1> tickArgs{counted};
                ui.label(
                    translator.formatted(
                        MessageId::GameToolbarTick, tickArgs));

                // Last on the row rather than beside the zoom buttons.
                // Every widget declared before it then keeps its place.
                // So a session recorded before this replays untouched.
                ui.button(
                    translator.text(MessageId::GameToolbarMenu),
                    {.id = widgets::kMenu});
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
                        translator.text(toolLabel(tool)),
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
