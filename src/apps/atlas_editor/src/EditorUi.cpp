#include "antwika/atlas_editor/EditorUi.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/HoverPointer.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Overlays.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/SliderSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/EditorTheme.hpp"
#include "antwika/atlas_editor/FileList.hpp"
#include "antwika/atlas_editor/Ink.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::ui::Alignment;
    using antwika::ui::ButtonState;
    using antwika::ui::ContainerSpec;
    using antwika::ui::Context;
    using antwika::ui::DrawText;
    using antwika::ui::FillRect;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::Theme;

    namespace
    {
        constexpr std::uint32_t kSwatchPixels = 18;

        constexpr std::uint32_t kSwatchRing = 2;

        constexpr std::uint32_t kModalWidth = 420;

        constexpr std::uint32_t kAtlasModalWidth = 520;

        constexpr std::size_t kFieldsPerRow = 2;

        constexpr std::string_view kInkLabel = "ink";

        constexpr Color kScrim{
            .red = 8, .green = 9, .blue = 12, .alpha = 220};

        [[nodiscard]] std::string sizeText(const Size size)
        {
            return std::to_string(size.width) + "x"
                   + std::to_string(size.height);
        }

        constexpr std::string_view kNoSlot = "-";

        [[nodiscard]] std::array<std::string, kFileItemCount>
        fileMenuTexts(const Translator &translator)
        {
            return {
                translator.text(MessageId::New),
                translator.text(MessageId::Save),
                translator.text(MessageId::Load),
                translator.text(MessageId::Quit)};
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::array<std::string, kAtlasPresetCount>
        presetTexts(const Translator &translator)
        {
            std::array<std::string, kAtlasPresetCount> named{};

            for (std::size_t preset = 0; preset < kAtlasPresetCount;
                 ++preset)
            {
                named[preset] = translator.text(presetNameId(preset));
            }

            return named;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::array<std::string, kViewItemCount>
        viewMenuTexts(const Translator &translator)
        {
            return {
                translator.text(MessageId::ZoomIn),
                translator.text(MessageId::ZoomOut),
                translator.text(MessageId::ResetView),
                translator.text(MessageId::Grid),
                translator.text(MessageId::Guides),
                translator.text(MessageId::Pivot),
                translator.text(MessageId::PixelGrid),
                translator.text(MessageId::PointerBorder),
                translator.text(MessageId::Preview),
                translator.text(MessageId::PreviewFocus)};
        } // GCOVR_EXCL_LINE

        template <std::size_t Count>
        [[nodiscard]] std::array<std::string_view, Count> viewsOf(
            const std::array<std::string, Count> &texts)
        {
            std::array<std::string_view, Count> named{};

            for (std::size_t at = 0; at < Count; ++at)
            {
                named[at] = texts[at];
            }

            return named;
        }

        [[nodiscard]] bool covers(
            const Rect &rect, const Point point) noexcept
        {
            const auto right = rect.origin.x
                + static_cast<std::int32_t>(rect.size.width);
            const auto bottom = rect.origin.y
                + static_cast<std::int32_t>(rect.size.height);

            return point.x >= rect.origin.x && point.x < right
                   && point.y >= rect.origin.y && point.y < bottom;
        }

        [[nodiscard]] bool underAnOverlay(
            const Frame &frame, const Pointer pointer) noexcept
        {
            return antwika::ui::overlaid(
                frame.overlays,
                antwika::ui::HoverPointer{.position = pointer.position});
        }

        void paintTip(
            Frame &frame,
            const Theme &theme,
            const Rect &over,
            const std::string &text)
        {
            const auto extent =
                antwika::gfx::textSize(text, theme.textScale);

            const Rect box{
                .origin = {
                    .x = over.origin.x
                        + static_cast<std::int32_t>(over.size.width)
                        + static_cast<std::int32_t>(theme.gap),
                    .y = over.origin.y},
                .size = {
                    .width = extent.width + theme.padding * 2,
                    .height = extent.height + theme.padding * 2}};

            frame.commands.push_back(
                FillRect{.rect = box, .color = theme.buttonPressed});

            frame.commands.push_back(
                DrawText{ // GCOVR_EXCL_LINE
                    .origin = {
                        .x = box.origin.x
                            + static_cast<std::int32_t>(theme.padding),
                        .y = box.origin.y
                            + static_cast<std::int32_t>(theme.padding)},
                    .text = text, // GCOVR_EXCL_LINE
                    .scale = theme.textScale,
                    .color = theme.buttonText});
        }

        void addTooltip(
            Frame &frame,
            const Theme &theme,
            const Pointer pointer,
            const Translator &translator)
        {
            if (underAnOverlay(frame, pointer))
            {
                return;
            }

            const auto where = pointer.position.value_or(
                Point{.x = -1, .y = -1});

            for (std::size_t index = 0; index < kToolCount; ++index)
            {
                const auto tool = static_cast<Tool>(index);
                const auto rect =
                    frame.rects.find(widgets::toolWidget(tool))
                        .value_or(Rect{});

                if (covers(rect, where))
                {
                    paintTip(
                        frame,
                        theme,
                        rect,
                        translator.text(toolNameId(tool)));

                    return;
                }
            }
        }

        void appendInkPicker(Context &ui, const EditorState &state)
        {
            const auto card = ui.panel(
                {.width = kGrow,
                 .height = kFit,
                 .id = widgets::kInkPanel});

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.label(kInkLabel);
                ui.spacer(kGrow);

                ContainerSpec swatch{
                    .width = fixedSize(kSwatchPixels),
                    .height = fixedSize(kSwatchPixels),
                    .padding = 0,
                    .gap = 0,
                    .id = widgets::kInkSwatch};
                swatch.background = state.color();

                const auto shown = ui.panel(swatch);
            }

            for (std::size_t channel = 0; channel < kInkChannels;
                 ++channel)
            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.label(kInkNames[channel], ui.theme().muted);

                ui.slider(
                    {.id = widgets::inkChannelWidget(channel),
                     .width = kGrow,
                     .value = inkChannelOf(state.color(), channel),
                     .range = kInkRange,
                     .dragging = state.inkDrag() == channel});
            }
        }

        [[nodiscard]] MessageId kindNameId(const AtlasKind kind) noexcept
        {
            return kind == AtlasKind::Isometric
                       ? MessageId::KindIsometric
                       : MessageId::KindFlat;
        }

        void fileCard(
            Context &ui,
            const EditorState &state,
            const Translator &translator)
        {
            const bool saving = state.openModal() == Modal::Save;

            ui.label(
                translator.text(
                    saving ? MessageId::Save : MessageId::Load));

            ui.label(state.directory(), ui.theme().muted);

            if (saving)
            {
                for (const auto &line :
                     metaLines(state.meta(), translator))
                {
                    ui.label(line, ui.theme().muted);
                }
            }

            const auto from = state.fileScroll();
            const auto until = std::min(
                from + filesShownIn(
                    state.canvas(), labelsAbove(state.openModal())),
                state.files().size());

            for (std::size_t at = from; at < until; ++at)
            {
                ui.button(
                    entryText(state.files()[at]),
                    {.id = widgets::fileEntryWidget(at),
                     .width = kGrow});
            }

            ui.textField(
                {.id = widgets::kFileField,
                 .width = kGrow,
                 .text = state.fileName(),
                 .placeholder = translator.text(MessageId::FileName),
                 .cursor = state.fileCaret()});

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(
                        saving ? MessageId::Save : MessageId::Load),
                    {.id = widgets::kFileConfirm});

                ui.button(
                    translator.text(MessageId::Close),
                    {.id = widgets::kFileClose});
            }
        }

        [[nodiscard]] Frame describeFileModal(
            const EditorState &state,
            const Theme &theme,
            const Pointer pointer,
            const Translator &translator,
            const antwika::ui::Keyboard &keyboard)
        {
            Context ui{
                state.canvas(),
                theme,
                pointer,
                keyboard,
                widgets::kFileField};

            {
                const auto screen = ui.panel(
                    {.width = kGrow,
                     .height = kGrow,
                     .cross = Alignment::Center,
                     .background = kScrim});

                ui.spacer(kGrow);

                {
                    const auto card = ui.panel(
                        {.width = fixedSize(kModalWidth),
                         .height = kFit});

                    fileCard(ui, state, translator);
                }

                ui.spacer(kGrow);
            }

            return ui.finish();
        } // GCOVR_EXCL_LINE

        void atlasField(
            Context &ui,
            const EditorState &state,
            const Translator &translator,
            const std::size_t field)
        {
            ui.label(
                translator.text(
                    fieldNameId(static_cast<AtlasField>(field))),
                ui.theme().muted);

            ui.textField(
                {.id = widgets::atlasFieldWidget(field),
                 .width = kGrow,
                 .text = state.form().values[field],
                 .cursor = state.formField() == field
                               ? state.formCaret()
                               : 0});
        }

        void atlasCard(
            Context &ui,
            const EditorState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::NewAtlas));

            const auto presets = presetTexts(translator);

            ui.dropdown(
                {.id = widgets::kAtlasPresets,
                 .optionIdBase = widgets::kFirstAtlasPreset,
                 .width = kGrow,
                 .options = viewsOf(presets),
                 .placeholder = translator.text(MessageId::Presets),
                 .open = state.openMenu() == Menu::Preset});

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.label(
                    translator.text(MessageId::AtlasKind),
                    ui.theme().muted);

                ui.button(
                    translator.text(kindNameId(state.form().kind)),
                    {.id = widgets::kAtlasKind, .width = kGrow});
            }

            for (std::size_t field = 0; field < kAtlasFieldCount;
                 field += kFieldsPerRow)
            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                for (std::size_t at = 0; at < kFieldsPerRow; ++at)
                {
                    atlasField(ui, state, translator, field + at);
                }
            }

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(MessageId::Create),
                    {.id = widgets::kAtlasCreate});

                ui.button(
                    translator.text(MessageId::Close),
                    {.id = widgets::kFileClose});
            }
        }

        [[nodiscard]] Frame describeAtlasModal(
            const EditorState &state,
            const Theme &theme,
            const Pointer pointer,
            const Translator &translator,
            const antwika::ui::Keyboard &keyboard)
        {
            Context ui{
                state.canvas(),
                theme,
                pointer,
                keyboard,
                widgets::atlasFieldWidget(state.formField())};

            {
                const auto screen = ui.panel(
                    {.width = kGrow,
                     .height = kGrow,
                     .cross = Alignment::Center,
                     .background = kScrim});

                ui.spacer(kGrow);

                {
                    const auto card = ui.panel(
                        {.width = fixedSize(kAtlasModalWidth),
                         .height = kFit});

                    atlasCard(ui, state, translator);
                }

                ui.spacer(kGrow);
            }

            return ui.finish();
        } // GCOVR_EXCL_LINE

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
    }

    std::array<std::string, kMetaLines> metaLines(
        const AtlasMeta &meta, const Translator &translator)
    {
        const std::string columns = std::to_string(meta.columns);
        const std::string rows = std::to_string(meta.rows);
        const std::string spriteW = std::to_string(meta.sprite.width);
        const std::string spriteH = std::to_string(meta.sprite.height);
        const std::string pivotX = std::to_string(meta.pivot.x);
        const std::string pivotY = std::to_string(meta.pivot.y);
        const std::string isoW = std::to_string(meta.isometric.width);
        const std::string isoH = std::to_string(meta.isometric.height);
        const std::string kind =
            translator.text(kindNameId(meta.kind));

        const std::array<std::string_view, 2> slots{columns, rows};
        const std::array<std::string_view, 2> sprite{spriteW, spriteH};
        const std::array<std::string_view, 2> pivot{pivotX, pivotY};
        const std::array<std::string_view, 2> isometric{isoW, isoH};
        const std::array<std::string_view, 1> named{kind};

        return {
            translator.formatted(MessageId::MetaSlots, slots),
            translator.formatted(MessageId::MetaSprite, sprite),
            translator.formatted(MessageId::MetaPivot, pivot),
            translator.formatted(MessageId::MetaIsometric, isometric),
            translator.formatted(MessageId::MetaKind, named)};
    } // GCOVR_EXCL_LINE

    std::string statusLine(
        const EditorState &state, const Translator &translator)
    {
        std::string line =
            translator.text(toolNameId(state.tool())) + "  "
            + whereText(state, translator) + "  x"
            + std::to_string(scaleOf(state.view())) + "  "
            + sizeText(state.image().size());

        if (const auto marked = state.shownSelection(); marked)
        {
            const std::string across =
                std::to_string(marked->size.width);
            const std::string down =
                std::to_string(marked->size.height);
            const std::array<std::string_view, 2> extent{across, down};

            line += "  "
                    + translator.formatted(
                        MessageId::SelectionSize, extent);
        }

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

    namespace
    {
        void appendPanes(Context &ui, const EditorState &state)
        {
            if (!state.preview().open)
            {
                const auto sheet = ui.column(
                    {.width = kGrow,
                     .height = kGrow,
                     .id = widgets::kSheetPane});

                return;
            }

            const auto pair = ui.split(
                {.id = widgets::kPreviewDivider,
                 .axis = antwika::ui::Axis::Row,
                 .ratio = state.preview().ratio,
                 .minimum = kPaneMinimum,
                 .dragging = state.preview().dragging});

            {
                const auto sheet = ui.column(
                    {.width = kGrow,
                     .height = kGrow,
                     .id = widgets::kSheetPane});
            }

            {
                const auto preview = ui.column(
                    {.width = kGrow,
                     .height = kGrow,
                     .id = widgets::kPreviewPane});
            }
        }
    }

    Frame describeEditor(
        const EditorState &state,
        const Pointer pointer,
        const Translator &translator,
        const antwika::ui::Keyboard &keyboard)
    {
        const Size canvas = state.canvas();
        const Theme theme = editorTheme(canvas);

        if (state.openModal() == Modal::New)
        {
            return describeAtlasModal(
                state, theme, pointer, translator, keyboard);
        }

        if (state.openModal() != Modal::None)
        {
            return describeFileModal(state, theme, pointer, translator,
                                     keyboard);
        }

        Context ui{canvas, theme, pointer};

        const auto fileTexts = fileMenuTexts(translator);
        const auto viewTexts = viewMenuTexts(translator);

        const auto fileItems = viewsOf(fileTexts);
        const auto viewItems = viewsOf(viewTexts);

        const auto fileName = translator.text(MessageId::MenuFile);
        const auto viewName = translator.text(MessageId::MenuView);

        {
            const auto bar = ui.panel({.width = kGrow, .height = kFit});

            const auto row =
                ui.row({.width = kGrow, .cross = Alignment::Center});

            ui.dropdown(
                {.id = widgets::kFileMenu,
                 .optionIdBase = widgets::kFirstFileItem,
                 .options = fileItems,
                 .placeholder = fileName,
                 .open = state.openMenu() == Menu::File});

            ui.dropdown(
                {.id = widgets::kViewMenu,
                 .optionIdBase = widgets::kFirstViewItem,
                 .options = viewItems,
                 .placeholder = viewName,
                 .open = state.openMenu() == Menu::View});

            ui.spacer(fixedSize(theme.gap * 2));

            const auto palette = defaultPalette();
            for (std::size_t index = 0; index < palette.size(); ++index)
            {
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

            ui.spacer(fixedSize(theme.gap * 2));

            ui.button(
                kInkLabel,
                {.id = widgets::kInkButton,
                 .state = state.inkVisible()
                              ? std::optional{ButtonState::Pressed}
                              : std::nullopt});

            ui.spacer(kGrow);
        }

        {
            const auto beside = ui.row({.width = kGrow, .height = kGrow});

            {
                const auto rail = ui.panel(
                    {.width = kFit,
                     .height = kGrow,
                     .id = widgets::kToolRail});

                for (std::size_t index = 0; index < kToolCount; ++index)
                {
                    const auto tool = static_cast<Tool>(index);

                    ui.button(
                        toolMark(tool),
                        {.id = widgets::toolWidget(tool),
                         .width = kGrow,
                         .state = tool == state.tool()
                                      ? std::optional{ButtonState::Pressed}
                                      : std::nullopt});
                }
            }

            appendPanes(ui, state);
        }

        {
            const auto foot = ui.panel(
                {.width = kGrow,
                 .height = kFit,
                 .id = widgets::kStatusBar});

            ui.label(statusLine(state, translator), theme.muted);
        }

        if (state.inkVisible())
        {
            appendInkPicker(ui, state);
        }

        auto frame = ui.finish();

        addTooltip(frame, theme, pointer, translator);

        return frame;
    }

}
