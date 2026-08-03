#include "antwika/atlas_editor/EditorUi.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
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

        // A slot nothing falls in is a dash rather than a word.
        // Notation, like the "px" and the "x" beside it.
        // So it stays a literal where every word is an id.
        constexpr std::string_view kNoSlot = "-";

        [[nodiscard]] std::string whereText(
            const EditorState &state, const Translator &translator)
        {
            const auto pixel = state.hovered();
            if (!pixel.has_value())
            {
                return translator.text(MessageId::PixelUnknown);
            }

            const auto slot =
                slotAt(state.tiles(), state.image().size(), *pixel);

            const std::string across = std::to_string(pixel->x);
            const std::string down = std::to_string(pixel->y);
            // Two statements rather than one conditional expression.
            // A temporary built on a branch has an unwinding path.
            // Which is a branch no test can reach and the gate refuses.
            std::string which{kNoSlot};

            if (slot.has_value())
            {
                which = std::to_string(*slot);
            }

            const std::array<std::string_view, 2> at{across, down};
            const std::array<std::string_view, 1> in{which};

            return translator.formatted(MessageId::PixelAt, at)
                   + "  "
                   + translator.formatted(MessageId::Slot, in);
        }
    } // namespace

    std::string statusLine(
        const EditorState &state, const Translator &translator)
    {
        std::string line =
            translator.text(toolNameId(state.tool())) + "  "
            + whereText(state, translator) + "  x"
            + std::to_string(scaleOf(state.view())) + "  "
            + sizeText(state.image().size());

        if (state.unsaved())
        {
            line += "  " + translator.text(MessageId::Unsaved);
        }

        if (state.status().has_value())
        {
            const std::array<std::string_view, 1> detail{
                state.status()->detail};

            line += "  "
                    + translator.formatted(state.status()->id, detail);
        }

        return line;
    } // GCOVR_EXCL_LINE

    Frame describeEditor(
        const EditorState &state,
        const Pointer pointer,
        const Translator &translator)
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
                        translator.text(toolNameId(tool)),
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
                ui.button(
                    translator.text(MessageId::ResetView),
                    {.id = widgets::kResetView});
                ui.button(
                    translator.text(MessageId::Grid),
                    {.id = widgets::kGrid,
                     .state = state.gridVisible()
                                  ? std::optional{ButtonState::Pressed}
                                  : std::nullopt});
                ui.button(
                    translator.text(MessageId::Guides),
                    {.id = widgets::kGuides,
                     .state = state.guidesVisible()
                                  ? std::optional{ButtonState::Pressed}
                                  : std::nullopt});
                ui.button(
                    translator.text(MessageId::Load),
                    {.id = widgets::kLoad});
                ui.button(
                    translator.text(MessageId::Save),
                    {.id = widgets::kSave});
            }

            {
                const auto row = ui.row({.width = kGrow});
                ui.label(statusLine(state, translator), theme.muted);
            }
        }

        return ui.finish();
    }

} // namespace antwika::atlas_editor
