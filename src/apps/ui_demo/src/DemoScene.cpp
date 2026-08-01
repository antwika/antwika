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
#include <antwika/i18n/MessageId.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/ui_demo/DemoMessage.hpp"
#include "antwika/ui_demo/Widgets.hpp"

namespace antwika::ui_demo
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::i18n::MessageId;
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
    using antwika::ui::TextFieldSpec;
    using antwika::ui::Theme;

    namespace
    {
        // The window behind the UI, which is not the theme's business.
        constexpr Color kBackdrop{.red = 10, .green = 12, .blue = 18};

        // A colour of the caller's own, to show a label takes one.
        constexpr Color kInk{.red = 120, .green = 200, .blue = 255};

        // Three fills for the layout page's three columns.
        constexpr Color kFirstFill{.red = 34, .green = 40, .blue = 52};
        constexpr Color kSecondFill{.red = 44, .green = 52, .blue = 40};
        constexpr Color kThirdFill{.red = 52, .green = 38, .blue = 48};

        // What each accent option paints its line in.
        constexpr std::array<Color, kAccentCount> kAccentInks{
            Color{.red = 244, .green = 180, .blue = 60},
            Color{.red = 120, .green = 230, .blue = 170},
            Color{.red = 240, .green = 120, .blue = 150}};

        // Pixel counts, every one of them arbitrary on purpose.
        // What a page shows is the sizing rule, not the number.
        constexpr std::uint32_t kPickerWidth = 320;
        constexpr std::uint32_t kFixedButton = 120;
        constexpr std::uint32_t kSwatch = 24;
        constexpr std::uint32_t kSqueezedWidth = 180;
        constexpr std::uint32_t kWideButton = 220;
        constexpr std::uint32_t kMarkerHeight = 4;
        constexpr std::uint32_t kLamp = 10;
        constexpr std::uint32_t kWideGap = 12;

        // Where a widget-rects marker is painted.
        constexpr Color kMarkerInk{
            .red = 244, .green = 208, .blue = 63};

        // The lamp's two states, lit while the pointer is over the UI.
        constexpr Color kLampOn{.red = 120, .green = 230, .blue = 170};
        constexpr Color kLampOff{.red = 40, .green = 44, .blue = 52};

        /**
         * @brief Get the colour one accent option stands for.
         * @param index The option selected, if any.
         * @param theme The theme to fall back on.
         * @return The accent's colour, or ordinary text while nothing
         * is selected.
         */
        [[nodiscard]] Color accentInk(
            const std::size_t index, const Theme &theme) noexcept
        {
            if (index >= kAccentCount)
            {
                return theme.text;
            }

            return kAccentInks[index];
        }

        /**
         * @brief One list of option names, and views onto them.
         *
         * ui::DropdownSpec borrows what it is handed, so a translated
         * name has to outlive the Context it is declared into -- which
         * a vector of temporaries would not.
         * Keeping the strings and the views in one value is what makes
         * that one local rather than two that could part company.
         *
         * It is filled in place rather than returned by a function of
         * its own: an aggregate holding strings is destroyed on that
         * function's closing brace, which is an unwinding path no test
         * can reach and a line the coverage gate then refuses.
         */
        template <std::size_t Count>
        struct Options final
        {
            std::array<std::string, Count> names;
            std::array<std::string_view, Count> views;

            /**
             * @brief Put one option's name in, and a view onto it.
             * @param index Which option.
             * @param text What it is called.
             */
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
            const DemoState & /*state*/,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoLabelsLine));
            ui.label(
                translator.text(MessageId::UiDemoLabelsMuted),
                ui.theme().muted);
            ui.label(translator.text(MessageId::UiDemoLabelsOwnInk), kInk);

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.label(translator.text(MessageId::UiDemoSpacerLeft));
                ui.spacer(kGrow);
                ui.label(translator.text(MessageId::UiDemoSpacerRight));
            }
        }

        void describeButtons(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoButtonsPress));

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.button(
                    translator.text(MessageId::UiDemoButtonCount),
                    {.id = widgets::kCount});
                ui.button(
                    translator.text(MessageId::UiDemoButtonReset),
                    {.id = widgets::kReset});
                const std::string count =
                    std::to_string(state.clicks());
                const std::array<std::string_view, 1> counted{count};

                ui.label(translator.formatted(
                    MessageId::UiDemoPressedCount, counted));
            }

            ui.label(translator.text(MessageId::UiDemoButtonsForced));

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(MessageId::UiDemoButtonIdle),
                    {.state = ButtonState::Idle});
                ui.button(
                    translator.text(MessageId::UiDemoButtonHovered),
                    {.state = ButtonState::Hovered});
                ui.button(
                    translator.text(MessageId::UiDemoButtonPressed),
                    {.state = ButtonState::Pressed});

                // Unnamed, so nothing can hover or activate it.
                ui.button(translator.text(MessageId::UiDemoButtonUnnamed));
            }

            ui.label(translator.text(MessageId::UiDemoButtonsWidths));

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(MessageId::UiDemoButtonFit),
                    {.width = kFit});
                ui.button(
                    translator.text(MessageId::UiDemoButtonFixed),
                    {.width = fixedSize(kFixedButton)});
                ui.button(
                    translator.text(MessageId::UiDemoButtonGrow),
                    {.width = kGrow});
            }
        }

        void describeLayout(
            Context &ui,
            const DemoState & /*state*/,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoLayoutNest));

            {
                const auto row =
                    ui.row({.width = kGrow, .gap = kWideGap});

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::Start,
                         .background = kFirstFill,
                         .padding = kWideGap});

                    ui.label(translator.text(MessageId::UiDemoAlignStart));
                    ui.label(translator.text(MessageId::UiDemoAcrossAxis));
                }

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::Center,
                         .background = kSecondFill,
                         .padding = kWideGap});

                    ui.label(translator.text(MessageId::UiDemoAlignCenter));
                    ui.label(translator.text(MessageId::UiDemoAcrossAxis));
                }

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::End,
                         .background = kThirdFill,
                         .padding = kWideGap,
                         .gap = kWideGap});

                    ui.label(translator.text(MessageId::UiDemoAlignEnd));
                    ui.label(translator.text(MessageId::UiDemoAcrossAxis));
                }
            }

            {
                const auto inner = ui.panel({.width = kGrow});

                ui.label(translator.text(MessageId::UiDemoPanelIsColumn));
                ui.label(translator.text(MessageId::UiDemoPanelInset));
            }
        }

        void describeTextField(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoFieldOwned));

            // The placeholder is borrowed by the spec.
            // So it is a named local rather than a temporary.
            const std::string prompt =
                translator.text(MessageId::UiDemoFieldPlaceholder);

            ui.textField(TextFieldSpec{
                .id = widgets::kField,
                .width = kGrow,
                .text = state.text(),
                .placeholder = prompt,
                .cursor = state.caret()});

            ui.label(translator.text(MessageId::UiDemoFieldKeys));
            const std::array<std::string_view, 1> held{state.text()};

            ui.label(
                translator.formatted(
                    MessageId::UiDemoFieldHolding, held),
                ui.theme().muted);
        }

        void describeDropdown(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoListOpenBit));

            // The spec borrows its options and its placeholder.
            // A translated name is a std::string rather than a literal.
            // So both are named locals living past ui.finish().
            OptionNames names;

            for (std::size_t index = 0; index < kAccentCount; ++index)
            {
                names.set(index, translator.text(accentNameId(index)));
            }

            const std::string empty =
                translator.text(MessageId::UiDemoNoneChosen);

            ui.dropdown(DropdownSpec{
                .id = widgets::kPalette,
                .optionIdBase = widgets::kFirstAccent,
                .width = fixedSize(kPickerWidth),
                .options = names.views,
                .selected = state.accent(),
                .placeholder = empty,
                .open = state.accentOpen()});

            ui.label(
                translator.text(MessageId::UiDemoListOverlay),
                accentInk(state.accent(), ui.theme()));
        }

        void describeFocus(
            Context &ui,
            const DemoState &state,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoFocusKeys));

            {
                const auto row = ui.row({.width = kGrow});

                ui.button(
                    translator.text(MessageId::UiDemoButtonFirst),
                    {.id = widgets::kFirst});
                ui.button(
                    translator.text(MessageId::UiDemoButtonSecond),
                    {.id = widgets::kSecond});
                ui.button(
                    translator.text(MessageId::UiDemoButtonThird),
                    {.id = widgets::kThird});
            }

            ui.label(translator.text(MessageId::UiDemoFocusRingFills));
            const std::string id = std::to_string(
                static_cast<std::uint64_t>(state.focus()));
            const std::array<std::string_view, 1> focused{id};

            ui.label(
                translator.formatted(
                    MessageId::UiDemoFocusedId, focused),
                ui.theme().muted);
        }

        // The swatch labels are ui::Theme's own field names.
        // Which is why they are the one thing here left in English.
        // A translated one would lie about what a caller types.
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
            const DemoState & /*state*/,
            const Translator &translator)
        {
            const auto &theme = ui.theme();

            ui.label(translator.text(MessageId::UiDemoThemeColours));

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

            // Scaled for this canvas, so these are the numbers in use.
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
            const DemoState & /*state*/,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoRectsSays));

            {
                const auto row = ui.row(
                    {.width = kGrow,
                     .cross = Alignment::Center,
                     .id = widgets::kMarked});

                ui.label(translator.text(MessageId::UiDemoRowIsNamed));
                ui.spacer(kGrow);
                ui.label(translator.text(MessageId::UiDemoBarFromRect));
            }

            ui.label(translator.text(MessageId::UiDemoUndeclaredId));
        }

        void describeShrink(
            Context &ui,
            const DemoState & /*state*/,
            const Translator &translator)
        {
            ui.label(translator.text(MessageId::UiDemoShrinkProportion));

            {
                const auto strip = ui.row(
                    {.width = fixedSize(kSqueezedWidth),
                     .id = widgets::kSqueezed});

                ui.button(
                    translator.text(MessageId::UiDemoTooWide),
                    {.width = fixedSize(kWideButton)});
                ui.button(
                    translator.text(MessageId::UiDemoAlsoTooWide),
                    {.width = fixedSize(kWideButton)});
            }

            ui.label(translator.text(MessageId::UiDemoNoClipping));
            ui.label(translator.text(MessageId::UiDemoLayoutsJob));
        }

        using Page = void (*)(
            Context &, const DemoState &, const Translator &);

        // One entry per Showcase, in the enumeration's own order.
        // A page added there therefore has to be written here.
        constexpr std::array<Page, kShowcaseCount> kPages{
            &describeLabels,
            &describeButtons,
            &describeLayout,
            &describeTextField,
            &describeDropdown,
            &describeFocus,
            &describeTheme,
            &describeRects,
            &describeShrink};

        /**
         * @brief Word the last thing the demo said, if it said one.
         *
         * The argument is a datum or a second id and never both, which
         * is the one branch DemoMessage's two fields buy.
         *
         * @param state Holds the message, or none.
         * @param translator Words it.
         * @return The line, empty while nothing has been said.
         */
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

        /**
         * @brief Paint a bar across whatever a named widget occupies.
         *
         * The point of the exercise: art placed *from* the layout rather
         * than beside it, which is what ui::WidgetRects exists for.
         * An id this frame did not declare draws nothing, since
         * inventing a rectangle for it is the second layout this avoids.
         *
         * @param frame The frame to mark, whose rects are read.
         * @param id The widget to mark out.
         */
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

        /**
         * @brief Light a corner while the pointer is over the UI.
         *
         * Appended after the layout rather than declared inside it,
         * because what it reports is decided while the frame is being
         * resolved: a widget saying it would always be one frame behind.
         *
         * @param frame The frame to light, whose interactions are read.
         */
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
    } // namespace

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
        // Both lists borrow their options, so both outlive the Context.
        PageNames pages;

        for (std::size_t index = 0; index < kShowcaseCount; ++index)
        {
            pages.set(
                index,
                translator.text(
                    showcaseNameId(static_cast<Showcase>(index))));
        }

        const std::string prompt =
            translator.text(MessageId::UiDemoPickPage);

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

                    ui.label(translator.text(MessageId::UiDemoTitle));
                    ui.spacer(kGrow);

                    // The picker is itself one of the elements shown.
                    // Its open flag lives in DemoState like any other.
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
                // Named, so the page's own area is reported back.
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

        // One id the page in question declares, one no page ever does.
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

} // namespace antwika::ui_demo
