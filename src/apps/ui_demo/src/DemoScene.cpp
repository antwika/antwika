#include "antwika/ui_demo/DemoScene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>

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

        void describeLabels(Context &ui, const DemoState & /*state*/)
        {
            ui.label("label() draws one line in the theme's colour");
            ui.label("a muted line reads as an aside", ui.theme().muted);
            ui.label("and a caller may hand it its own", kInk);

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.label("a growing spacer");
                ui.spacer(kGrow);
                ui.label("pushes these apart");
            }
        }

        void describeButtons(Context &ui, const DemoState &state)
        {
            ui.label("a button activates on the press, not a release");

            {
                const auto row = ui.row(
                    {.width = kGrow, .cross = Alignment::Center});

                ui.button("count", {.id = widgets::kCount});
                ui.button("reset", {.id = widgets::kReset});
                ui.label("pressed " + std::to_string(state.clicks()));
            }

            ui.label("an appearance can be forced by the caller");

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("idle", {.state = ButtonState::Idle});
                ui.button("hovered", {.state = ButtonState::Hovered});
                ui.button("pressed", {.state = ButtonState::Pressed});

                // Unnamed, so nothing can hover or activate it.
                ui.button("unnamed");
            }

            ui.label("and a width is fit, fixed or grow");

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("fit", {.width = kFit});
                ui.button("fixed", {.width = fixedSize(kFixedButton)});
                ui.button("grow", {.width = kGrow});
            }
        }

        void describeLayout(Context &ui, const DemoState & /*state*/)
        {
            ui.label("row, column and panel nest as deep as you like");

            {
                const auto row =
                    ui.row({.width = kGrow, .gap = kWideGap});

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::Start,
                         .background = kFirstFill,
                         .padding = kWideGap});

                    ui.label("start");
                    ui.label("across the axis");
                }

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::Center,
                         .background = kSecondFill,
                         .padding = kWideGap});

                    ui.label("center");
                    ui.label("across the axis");
                }

                {
                    const auto column = ui.column(
                        {.width = kGrow,
                         .cross = Alignment::End,
                         .background = kThirdFill,
                         .padding = kWideGap,
                         .gap = kWideGap});

                    ui.label("end");
                    ui.label("across the axis");
                }
            }

            {
                const auto inner = ui.panel({.width = kGrow});

                ui.label("a panel is a column with the theme's fill");
                ui.label("and the theme's inset, and nothing else");
            }
        }

        void describeTextField(Context &ui, const DemoState &state)
        {
            ui.label("the characters belong to the application");

            ui.textField(TextFieldSpec{
                .id = widgets::kField,
                .width = kGrow,
                .text = state.text(),
                .placeholder = "tab here and type",
                .cursor = state.caret()});

            ui.label("Enter submits it, Escape gives up on it");
            ui.label("holding: " + state.text(), ui.theme().muted);
        }

        void describeDropdown(Context &ui, const DemoState &state)
        {
            ui.label("whether a list is open is the caller's bit");

            ui.dropdown(DropdownSpec{
                .id = widgets::kPalette,
                .optionIdBase = widgets::kFirstAccent,
                .width = fixedSize(kPickerWidth),
                .options = accentOptions(),
                .selected = state.accent(),
                .placeholder = "none chosen",
                .open = state.accentOpen()});

            ui.label(
                "an open list is an overlay, hit first",
                accentInk(state.accent(), ui.theme()));
        }

        void describeFocus(Context &ui, const DemoState &state)
        {
            ui.label("Tab, Shift+Tab and Enter walk these three");

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("first", {.id = widgets::kFirst});
                ui.button("second", {.id = widgets::kSecond});
                ui.button("third", {.id = widgets::kThird});
            }

            ui.label("the ring is four fills, drawn after everything");
            ui.label(
                "focused id "
                    + std::to_string(
                        static_cast<std::uint64_t>(state.focus())),
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

        void describeTheme(Context &ui, const DemoState & /*state*/)
        {
            const auto &theme = ui.theme();

            ui.label("every colour a widget picks without being told");

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

        void describeRects(Context &ui, const DemoState & /*state*/)
        {
            ui.label("Frame::rects says where a named widget went");

            {
                const auto row = ui.row(
                    {.width = kGrow,
                     .cross = Alignment::Center,
                     .id = widgets::kMarked});

                ui.label("this row is named");
                ui.spacer(kGrow);
                ui.label("the bar is placed from its rect");
            }

            ui.label("an id no frame declared answers nothing at all");
        }

        void describeShrink(Context &ui, const DemoState & /*state*/)
        {
            ui.label("too little room shrinks children in proportion");

            {
                const auto strip = ui.row(
                    {.width = fixedSize(kSqueezedWidth),
                     .id = widgets::kSqueezed});

                ui.button(
                    "far too wide", {.width = fixedSize(kWideButton)});
                ui.button(
                    "also too wide", {.width = fixedSize(kWideButton)});
            }

            ui.label("there is no clipping, so containment is the");
            ui.label("layout's job rather than a renderer's");
        }

        using Page = void (*)(Context &, const DemoState &);

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

    Frame DemoScene::describe(
        const Size canvas,
        Pointer pointer,
        const Keyboard &keyboard,
        const DemoState &state) const
    {
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

                    ui.label("antwika::ui showcase");
                    ui.spacer(kGrow);

                    // The picker is itself one of the elements shown.
                    // Its open flag lives in DemoState like any other.
                    ui.dropdown(DropdownSpec{
                        .id = widgets::kPicker,
                        .optionIdBase = widgets::kFirstPage,
                        .width = fixedSize(kPickerWidth),
                        .options = showcaseOptions(),
                        .selected = state.selected(),
                        .placeholder = "pick an element",
                        .open = state.pickerOpen()});
                }
            }

            {
                // Named, so the page's own area is reported back.
                const auto card = ui.panel(
                    {.width = kGrow,
                     .height = kGrow,
                     .id = widgets::kCard});

                kPages[state.selected() % kShowcaseCount](ui, state);
            }

            ui.label(state.message(), ui.theme().muted);
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
