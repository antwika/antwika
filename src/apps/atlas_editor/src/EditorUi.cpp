#include "antwika/atlas_editor/EditorUi.hpp"

#include <cstddef>
#include <optional>
#include <string>

#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::ui::Alignment;
    using antwika::ui::ButtonState;
    using antwika::ui::Context;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        // Big enough to read a colour off at a glance.
        // Small enough that a dozen fit beside the buttons.
        constexpr std::uint32_t kSwatchPixels = 18;

        // The ring drawn round the selected swatch.
        constexpr std::uint32_t kSwatchRing = 2;

        [[nodiscard]] std::string sizeText(const Size size)
        {
            return std::to_string(size.width) + "x"
                   + std::to_string(size.height);
        }

        [[nodiscard]] std::string whereText(const EditorState &state)
        {
            const auto pixel = state.hovered();
            if (!pixel.has_value())
            {
                return "px -,-";
            }

            const auto slot =
                slotAt(state.tiles(), state.image().size(), *pixel);

            return "px " + std::to_string(pixel->x) + ","
                   + std::to_string(pixel->y) + "  slot "
                   + (slot.has_value()
                          ? std::to_string(*slot)
                          : std::string("-")); // GCOVR_EXCL_LINE
        }
    } // namespace

    std::string statusLine(const EditorState &state)
    {
        std::string line = std::string(toolName(state.tool())) + "  "
                           + whereText(state) + "  x"
                           + std::to_string(scaleOf(state.view())) + "  "
                           + sizeText(state.image().size());

        if (state.unsaved())
        {
            line += "  UNSAVED";
        }

        if (!state.status().empty())
        {
            line += "  " + state.status();
        }

        return line;
    } // GCOVR_EXCL_LINE

    Frame describeEditor(const EditorState &state, const Pointer pointer)
    {
        const Size canvas = state.canvas();
        const Theme theme =
            scaledTheme(Theme{}, scaleForCanvas(canvas));

        Context ui{canvas, theme, pointer};

        {
            const auto bar = ui.panel({.width = kGrow, .height = kFit});

            {
                const auto row =
                    ui.row({.width = kGrow, .cross = Alignment::Center});

                // One button per tool, in the enumeration's order.
                // A tool added there gets a button here.
                for (std::size_t index = 0; index < kToolCount; ++index)
                {
                    const auto tool = static_cast<Tool>(index);

                    ui.button(
                        std::string{toolName(tool)},
                        {.id = widgets::toolWidget(tool),
                         .state = tool == state.tool()
                                      ? std::optional{ButtonState::Pressed}
                                      : std::nullopt});
                }

                ui.spacer(fixedSize(theme.gap * 2));

                const auto palette = defaultPalette();
                for (std::size_t index = 0; index < palette.size();
                     ++index)
                {
                    // The ring is the outer fill seen through padding.
                    // antwika::ui draws no border of its own.
                    // Its focus ring is four rectangles for that reason.
                    const bool chosen = state.colorIndex() == index;
                    const auto ring = ui.panel(
                        {.width = fixedSize(kSwatchPixels + kSwatchRing * 2),
                         .height =
                             fixedSize(kSwatchPixels + kSwatchRing * 2),
                         .background = chosen ? theme.focusRing
                                              : theme.panel,
                         .padding = kSwatchRing,
                         .gap = 0});

                    const auto chip = ui.panel(
                        {.width = fixedSize(kSwatchPixels),
                         .height = fixedSize(kSwatchPixels),
                         .background = palette[index],
                         .padding = 0,
                         .gap = 0,
                         .id = widgets::swatchWidget(index)});
                }

                ui.spacer(kGrow);

                ui.button("-", {.id = widgets::kZoomOut});
                ui.button("+", {.id = widgets::kZoomIn});
                ui.button("fit", {.id = widgets::kResetView});
                ui.button(
                    "grid",
                    {.id = widgets::kGrid,
                     .state = state.gridVisible()
                                  ? std::optional{ButtonState::Pressed}
                                  : std::nullopt});
                ui.button("load", {.id = widgets::kLoad});
                ui.button("save", {.id = widgets::kSave});
            }

            {
                const auto row = ui.row({.width = kGrow});
                ui.label(statusLine(state), theme.muted);
            }
        }

        return ui.finish();
    }

} // namespace antwika::atlas_editor
