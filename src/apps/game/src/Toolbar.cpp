#include "antwika/game/Toolbar.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/game/MapView.hpp"
#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game
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
        // Two of them, so eleven tools are six rows rather than eleven.
        // One column of eleven wants more height than there is.
        // A container with too little room shrinks its children.
        // Which is a button whose caption no longer fits inside it.
        constexpr std::size_t kPaletteColumns = 2;

        // A palette button's width, in theme pixels.
        // Fixed rather than fitted, so the two columns line up.
        // A column as wide as its own longest caption is not one.
        constexpr std::uint32_t kToolCells = 48;

        // The closed game menu's width, in theme pixels.
        // Wide enough for the longest item it lists.
        // So the list lines up with the box rather than overhanging it.
        constexpr std::uint32_t kMenuCells = 70;

        // And the overlay menu's, on the same terms.
        // Wider, since its longest item is longer than that one's.
        constexpr std::uint32_t kViewCells = 90;
    } // namespace

    Toolbar::Toolbar(const Translator &translator) : translator(translator)
    {
    }

    Frame Toolbar::describe(
        Size canvas,
        Pointer pointer,
        const Camera &camera,
        std::optional<BuildTool> selected,
        bool paused,
        antwika::time::Tick tick,
        CityRatings ratings,
        bool menuOpen,
        MapView view,
        bool viewOpen,
        std::int64_t funds) const
    {
        const auto scale = scaleForCanvas(canvas);

        Context ui{canvas, scaledTheme(Theme{}, scale), pointer};

        // ui::DropdownSpec borrows its options rather than owning them.
        // So the words live here, for as long as the Context does.
        std::array<std::string, kMenuItemCount> words{};
        std::array<std::string_view, kMenuItemCount> items{};

        for (std::size_t index = 0; index < kMenuItemCount; ++index)
        {
            words[index] =
                translator.text(menuItemLabel(static_cast<MenuItem>(index)));
            items[index] = words[index];
        }

        std::array<std::string, kMapViewCount> viewWords{};
        std::array<std::string_view, kMapViewCount> viewItems{};

        for (std::size_t index = 0; index < kMapViewCount; ++index)
        {
            viewWords[index] =
                translator.text(mapViewLabel(static_cast<MapView>(index)));
            viewItems[index] = viewWords[index];
        }

        const std::uint32_t toolWidth = kToolCells * scale;
        const std::uint32_t paletteWidth = kPaletteColumns * toolWidth
                                           + ui.theme().gap
                                           + 2 * ui.theme().padding;

        {
            // Its own container rather than the Context's root.
            // The root spaces its children by the theme's gap.
            // These three are meant to meet the edges and each other.
            const auto screen = ui.column(
                {.width = kGrow,
                 .height = kGrow,
                 .padding = 0,
                 .gap = 0});

            {
                const auto bar = ui.panel(
                    {.width = kGrow,
                     .height = kFit,
                     .id = widgets::kTopBar});
                const auto row =
                    ui.row({.width = kGrow, .cross = Alignment::Center});

                // Whether the list is showing is the caller's to know.
                // A dropdown holds nothing between frames.
                // See ui::DropdownSpec, and here that is the point.
                // An open list changes what a click at a pixel means.
                ui.dropdown(
                    {.id = widgets::kGameMenu,
                     .optionIdBase = widgets::kFirstMenuItem,
                     .width = fixedSize(kMenuCells * scale),
                     .options = items,
                     .placeholder =
                         translator.text(MessageId::ToolbarGameMenu),
                     .open = menuOpen});

                // The one route to the menu modal, kept where it was.
                // F10 no longer opens it -- see UiSink.
                ui.button(
                    translator.text(MessageId::ToolbarMenu),
                    {.id = widgets::kMenu});

                // Which picture of the city is showing.
                // The closed box names it rather than saying "view".
                // So what is being looked at is readable at a glance.
                // Which is the one thing a placeholder cannot say.
                ui.dropdown(
                    {.id = widgets::kViewMenu,
                     .optionIdBase = widgets::kFirstViewItem,
                     .width = fixedSize(kViewCells * scale),
                     .options = viewItems,
                     .placeholder = translator.text(mapViewLabel(view)),
                     .open = viewOpen});

                ui.spacer(kGrow);
            }

            {
                const auto middle = ui.row(
                    {.width = kGrow,
                     .height = kGrow,
                     .padding = 0,
                     .gap = 0});

                // The city.
                // A spacer fills no pixels, so it covers none.
                // ui::Interactions::pointerOverUi is false over it.
                // Which is what leaves a press here to GridSink.
                ui.spacer(kGrow);

                {
                    const auto palette = ui.panel(
                        {.width = fixedSize(paletteWidth),
                         .height = kGrow,
                         .id = widgets::kSidePanel});

                    ui.label(translator.text(MessageId::ToolbarBuild));

                    // One button per tool, in the enumeration's order.
                    // A tool added there therefore gets a button here.
                    // And a row of its own once the columns are full.
                    for (std::size_t first = 0; first < kBuildToolCount;
                         first += kPaletteColumns)
                    {
                        const auto row = ui.row({.width = kGrow});

                        for (std::size_t column = 0;
                             column < kPaletteColumns;
                             ++column)
                        {
                            const auto index = first + column;

                            // An odd count leaves the last row short.
                            // Padded rather than left to close up.
                            // So a column stays a column.
                            if (index >= kBuildToolCount)
                            {
                                ui.spacer(fixedSize(toolWidth));
                                continue;
                            }

                            const auto tool =
                                static_cast<BuildTool>(index);

                            // The chosen one is held down.
                            // Which it is is then plain at a glance.
                            ui.button(
                                translator.text(toolLabel(tool)),
                                {.id = widgets::toolWidget(tool),
                                 .width = fixedSize(toolWidth),
                                 .state =
                                     tool == selected
                                         ? std::optional{
                                               ButtonState::Pressed}
                                         : std::nullopt});
                        }
                    }

                    ui.spacer(kGrow);
                }
            }

            {
                const auto bar = ui.panel(
                    {.width = kGrow,
                     .height = kFit,
                     .id = widgets::kBottomBar});
                const auto row =
                    ui.row({.width = kGrow, .cross = Alignment::Center});

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

                // The readouts are pushed to the far end.
                // A bar that gains a button does not move them along.
                ui.spacer(kGrow);

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
                        MessageId::ToolbarTick, tickArgs));

                // Labels rather than buttons, and not by omission.
                // There is nothing to press here, so there is no id.
                // And so a rating can never become an input.
                const auto people = std::to_string(ratings.population);
                const std::array<std::string_view, 1> peopleArgs{people};
                ui.label(
                    translator.formatted(
                        MessageId::ToolbarPopulation, peopleArgs));

                const auto jobs = std::to_string(ratings.employment);
                const std::array<std::string_view, 1> jobArgs{jobs};
                ui.label(
                    translator.formatted(
                        MessageId::ToolbarEmployment, jobArgs));

                // The bank, on the ratings' terms exactly.
                // Spent inside the tick path from recorded clicks.
                // So a replay draws the same number -- see GameState.
                const auto bank = std::to_string(funds);
                const std::array<std::string_view, 1> bankArgs{bank};
                ui.label(
                    translator.formatted(
                        MessageId::ToolbarMoney, bankArgs));
            }
        }

        return ui.finish();
    }

} // namespace antwika::game
