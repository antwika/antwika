#include "antwika/ui_demo/DemoScene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextAreaSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/ui_demo/DemoMessage.hpp"
#include "antwika/ui_demo/MessageId.hpp"
#include "antwika/ui_demo/Messages.hpp"
#include "antwika/ui_demo/Widgets.hpp"

namespace antwika::ui_demo
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::ui::Alignment;
    using antwika::ui::ButtonState;
    using antwika::ui::Context;
    using antwika::ui::DropdownSpec;
    using antwika::ui::FillRect;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::TextAreaSpec;
    using antwika::ui::TextFieldSpec;
    using antwika::ui::Theme;

    namespace
    {
        constexpr Color kBackdrop{.red = 10, .green = 12, .blue = 18};

        constexpr Color kInk{.red = 120, .green = 200, .blue = 255};

        constexpr Color kFirstFill{.red = 34, .green = 40, .blue = 52};
        constexpr Color kSecondFill{.red = 44, .green = 52, .blue = 40};
        constexpr Color kThirdFill{.red = 52, .green = 38, .blue = 48};

        constexpr std::array<Color, kAccentCount> kAccentInks{
            Color{.red = 244, .green = 180, .blue = 60},
            Color{.red = 120, .green = 230, .blue = 170},
            Color{.red = 240, .green = 120, .blue = 150}};

        constexpr std::uint32_t kPickerWidth = 320;
        constexpr std::uint32_t kFixedButton = 120;
        constexpr std::uint32_t kSwatch = 24;
        constexpr std::uint32_t kSqueezedWidth = 180;
        constexpr std::uint32_t kWideButton = 220;
        constexpr std::uint32_t kMarkerHeight = 4;
        constexpr std::uint32_t kLamp = 10;
        constexpr std::uint32_t kWideGap = 12;

        constexpr Color kMarkerInk{
            .red = 244, .green = 208, .blue = 63};

        constexpr Color kLampOn{.red = 120, .green = 230, .blue = 170};
        constexpr Color kLampOff{.red = 40, .green = 44, .blue = 52};

        [[nodiscard]] Color accentInk(
            const std::size_t index, const Theme &theme) noexcept
        {
            if (index >= kAccentCount)
            {
                return theme.text;
            }

            return kAccentInks[index];
        }

        template <std::size_t Count>
        struct Options final
        {
            std::array<std::string, Count> names;
            std::array<std::string_view, Count> views;

            void set(const std::size_t index, std::string text)
            {
                names[index] = std::move(text);
                views[index] = names[index];
            }
        };

        using OptionNames = Options<kAccentCount>;
        using PageNames = Options<kShowcaseCount>;

        void describeLabels(
            Context &ui,
            const DemoState & ,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::LabelsLine));
            ui.label(
                translator.text(MessageId::LabelsMuted),
                ui.theme().muted);
            ui.label(translator.text(MessageId::LabelsOwnInk), kInk);

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.label(translator.text(MessageId::SpacerLeft));
                ui.spacer(kGrow);
                ui.label(translator.text(MessageId::SpacerRight));
            }
        }

        void describeButtons(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::ButtonsPress));

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.button(
                    translator.text(MessageId::ButtonCount),
                    {.id = widgets::kCount});
                ui.button(
                    translator.text(MessageId::ButtonReset),
                    {.id = widgets::kReset});
                const std::string count =
                    std::to_string(state.clicks());
                const std::array<std::string_view, 1> counted{count};

                ui.label(translator.formatted(
                    MessageId::PressedCount, counted));
            }

            ui.label(translator.text(MessageId::ButtonsForced));

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(MessageId::ButtonIdle),
                    {.state = ButtonState::Idle});
                ui.button(
                    translator.text(MessageId::ButtonHovered),
                    {.state = ButtonState::Hovered});
                ui.button(
                    translator.text(MessageId::ButtonPressed),
                    {.state = ButtonState::Pressed});

                ui.button(translator.text(MessageId::ButtonUnnamed));
            }

            ui.label(translator.text(MessageId::ButtonsWidths));

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(MessageId::ButtonFit),
                    {.width = kFit});
                ui.button(
                    translator.text(MessageId::ButtonFixed),
                    {.width = fixedSize(kFixedButton)});
                ui.button(
                    translator.text(MessageId::ButtonGrow),
                    {.width = kGrow});
            }
        }

        void describeLayout(
            Context &ui,
            const DemoState & ,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::LayoutNest));

            {
                const auto row =
                    ui.row({.width = kGrow, .gap = kWideGap});

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::Start,
                         .background = kFirstFill,
                         .padding = kWideGap});

                    ui.label(translator.text(MessageId::AlignStart));
                    ui.label(translator.text(MessageId::AcrossAxis));
                }

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::Center,
                         .background = kSecondFill,
                         .padding = kWideGap});

                    ui.label(translator.text(MessageId::AlignCenter));
                    ui.label(translator.text(MessageId::AcrossAxis));
                }

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::End,
                         .background = kThirdFill,
                         .padding = kWideGap,
                         .gap = kWideGap});

                    ui.label(translator.text(MessageId::AlignEnd));
                    ui.label(translator.text(MessageId::AcrossAxis));
                }
            }

            {
                const auto inner = ui.panel({.width = kGrow});

                ui.label(translator.text(MessageId::PanelIsColumn));
                ui.label(translator.text(MessageId::PanelInset));
            }
        }

        void describeTextField(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::FieldOwned));

            const std::string prompt =
                translator.text(MessageId::FieldPlaceholder);

            ui.textField(TextFieldSpec{
                .id = widgets::kField,
                .width = kGrow,
                .text = state.text(),
                .placeholder = prompt,
                .cursor = state.caret()});

            ui.label(translator.text(MessageId::FieldKeys));
            const std::array<std::string_view, 1> held{state.text()};

            ui.label(
                translator.formatted(
                    MessageId::FieldHolding, held),
                ui.theme().muted);
        }

        void describeDropdown(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::ListOpenBit));

            OptionNames names;

            for (std::size_t index = 0; index < kAccentCount; ++index)
            {
                names.set(index, translator.text(accentNameId(index)));
            }

            const std::string empty =
                translator.text(MessageId::NoneChosen);

            ui.dropdown(DropdownSpec{
                .id = widgets::kPalette,
                .optionIdBase = widgets::kFirstAccent,
                .width = fixedSize(kPickerWidth),
                .options = names.views,
                .selected = state.accent(),
                .placeholder = empty,
                .open = state.accentOpen()});

            ui.label(
                translator.text(MessageId::ListOverlay),
                accentInk(state.accent(), ui.theme()));
        }

        void describeFocus(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::FocusKeys));

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(MessageId::ButtonFirst),
                    {.id = widgets::kFirst});
                ui.button(
                    translator.text(MessageId::ButtonSecond),
                    {.id = widgets::kSecond});
                ui.button(
                    translator.text(MessageId::ButtonThird),
                    {.id = widgets::kThird});
            }

            ui.label(translator.text(MessageId::FocusRingFills));
            const std::string id = std::to_string(
                static_cast<std::uint64_t>(state.focus()));
            const std::array<std::string_view, 1> focused{id};

            ui.label(
                translator.formatted(
                    MessageId::FocusedId, focused),
                ui.theme().muted);
        }

        void describeSwatch(
            Context &ui, const std::string_view name, const Color ink)
        {
            const auto row =
                ui.row({.width = kGrow, .cross = Alignment::Center});

            {
                const auto chip = ui.column(
                    {.width = fixedSize(kSwatch),
                     .height = fixedSize(kSwatch),
                     .background = ink});
            }

            ui.label(std::string{name});
        }

        void describeTheme(
            Context &ui,
            const DemoState & ,
            const Translator &translator)
        {
            const auto &theme = ui.theme();

            ui.label(translator.text(MessageId::ThemeColours));

            describeSwatch(ui, "panel", theme.panel);
            describeSwatch(ui, "text", theme.text);
            describeSwatch(ui, "muted", theme.muted);
            describeSwatch(ui, "buttonIdle", theme.buttonIdle);
            describeSwatch(ui, "buttonHovered", theme.buttonHovered);
            describeSwatch(ui, "buttonPressed", theme.buttonPressed);
            describeSwatch(ui, "buttonText", theme.buttonText);
            describeSwatch(ui, "field", theme.field);
            describeSwatch(ui, "fieldFocused", theme.fieldFocused);
            describeSwatch(ui, "caret", theme.caret);
            describeSwatch(ui, "focusRing", theme.focusRing);

            ui.label(
                "textScale " + std::to_string(theme.textScale)
                    + "  padding " + std::to_string(theme.padding)
                    + "  gap " + std::to_string(theme.gap));
            ui.label(
                "buttonPadding " + std::to_string(theme.buttonPadding)
                    + "  focusRingThickness "
                    + std::to_string(theme.focusRingThickness));
        }

        void describeRects(
            Context &ui,
            const DemoState & ,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::RectsSays));

            {
                const auto row = ui.row(
                    {.width = kGrow,
                     .cross = Alignment::Center,
                     .id = widgets::kMarked});

                ui.label(translator.text(MessageId::RowIsNamed));
                ui.spacer(kGrow);
                ui.label(translator.text(MessageId::BarFromRect));
            }

            ui.label(translator.text(MessageId::UndeclaredId));
        }

        void describeShrink(
            Context &ui,
            const DemoState & ,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::ShrinkProportion));

            {
                const auto strip = ui.row(
                    {.width = fixedSize(kSqueezedWidth),
                     .id = widgets::kSqueezed});

                ui.button(
                    translator.text(MessageId::TooWide),
                    {.width = fixedSize(kWideButton)});
                ui.button(
                    translator.text(MessageId::AlsoTooWide),
                    {.width = fixedSize(kWideButton)});
            }

            ui.label(translator.text(MessageId::NoClipping));
            ui.label(translator.text(MessageId::LayoutsJob));
        }

        constexpr std::uint32_t kPaneHeight = 120;

        void describeTextArea(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::AreaOwned));

            const std::string prompt =
                translator.text(MessageId::AreaPlaceholder);

            ui.textArea(TextAreaSpec{
                .id = widgets::kArea,
                .width = kGrow,
                .height = fixedSize(kPaneHeight),
                .text = state.areaText(),
                .placeholder = prompt,
                .cursor = state.areaCursor(),
                .anchor = std::optional{state.areaAnchor()},
                .scroll = state.areaScroll(),
                .scrollbar = true,
                .focused = state.focus() == widgets::kArea,
                .dragging = state.areaDragging()});

            const std::string line =
                std::to_string(state.areaScroll());
            const std::array<std::string_view, 1> shown{line};

            ui.label(
                translator.formatted(MessageId::AreaShowing, shown),
                ui.theme().muted);
        }

        using Page = void (*)(
            Context &, const DemoState &, const Translator &);

        constexpr std::array<Page, kShowcaseCount> kPages{
            &describeLabels,
            &describeButtons,
            &describeLayout,
            &describeTextField,
            &describeDropdown,
            &describeFocus,
            &describeTheme,
            &describeRects,
            &describeShrink,
            &describeTextArea};

        [[nodiscard]] std::string lastSaid(
            const DemoState &state, const Translator &translator)
        {
            const auto &said = state.message();

            if (!said.has_value())
            {
                return {};
            }

            const std::string argument =
                said->argId.has_value()
                    ? translator.text(*said->argId)
                    : said->datum;
            const std::array<std::string_view, 1> args{argument};

            return translator.formatted(said->id, args);
        }

        void appendMarker(Frame &frame, const antwika::ui::WidgetId id)
        {
            const auto rect = frame.rects.find(id);

            if (!rect.has_value())
            {
                return;
            }

            const auto bottom = static_cast<std::int32_t>(
                static_cast<std::int64_t>(rect->origin.y)
                + rect->size.height);

            frame.commands.push_back(FillRect{
                .rect =
                    Rect{
                        .origin = {.x = rect->origin.x, .y = bottom},
                        .size = {
                            .width = rect->size.width,
                            .height = kMarkerHeight}},
                .color = kMarkerInk});
        }

        void appendLamp(Frame &frame)
        {
            const auto ink =
                frame.interactions.pointerOverUi ? kLampOn : kLampOff;

            frame.commands.push_back(FillRect{
                .rect =
                    Rect{
                        .origin = {.x = 0, .y = 0},
                        .size = {.width = kLamp, .height = kLamp}},
                .color = ink});
        }
    }

    DemoScene::DemoScene(const Translator &translator)
        : translator(translator)
    {
    }

    Frame DemoScene::describe(
        const Size canvas,
        Pointer pointer,
        const Keyboard &keyboard,
        const DemoState &state) const
    {
        PageNames pages;

        for (std::size_t index = 0; index < kShowcaseCount; ++index)
        {
            pages.set(
                index,
                translator.text(
                    showcaseNameId(static_cast<Showcase>(index))));
        }

        const std::string prompt =
            translator.text(MessageId::PickPage);

        Context ui{
            canvas,
            scaledTheme(Theme{}, scaleForCanvas(canvas)),
            pointer,
            keyboard,
            state.focus()};

        {
            const auto screen =
                ui.column({.width = kGrow, .height = kGrow});

            {
                const auto header = ui.panel({.width = kGrow});

                {
                    const auto row = ui.row(
                        {.width = kGrow, .cross = Alignment::Center});

                    ui.label(translator.text(MessageId::Title));
                    ui.spacer(kGrow);

                    ui.dropdown(DropdownSpec{
                        .id = widgets::kPicker,
                        .optionIdBase = widgets::kFirstPage,
                        .width = fixedSize(kPickerWidth),
                        .options = pages.views,
                        .selected = state.selected(),
                        .placeholder = prompt,
                        .open = state.pickerOpen()});
                }
            }

            {
                const auto card = ui.panel(
                    {.width = kGrow,
                     .height = kGrow,
                     .id = widgets::kCard});

                kPages[state.selected() % kShowcaseCount](
                ui, state, translator);
            }

            ui.label(lastSaid(state, translator), ui.theme().muted);
        }

        auto frame = ui.finish();

        appendMarker(frame, widgets::kMarked);
        appendMarker(frame, widgets::kNeverDeclared);
        appendLamp(frame);

        return frame;
    }

    void DemoScene::draw(
        IRenderer &renderer, const DrawList &picture) const
    {
        renderer.clear(kBackdrop);
        antwika::ui::paint(renderer, picture);
    }

}
