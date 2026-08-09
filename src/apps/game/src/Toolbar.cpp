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
        constexpr std::size_t kPaletteColumns = 2;

        static_assert(
            kBuildToolCount % kPaletteColumns == 0,
            "an odd tool count needs the palette's padding spacer back");

        constexpr std::uint32_t kToolCells = 48;

        constexpr std::uint32_t kMenuCells = 70;

        constexpr std::uint32_t kViewCells = 90;
    }

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

                ui.dropdown(
                    {.id = widgets::kGameMenu,
                     .optionIdBase = widgets::kFirstMenuItem,
                     .width = fixedSize(kMenuCells * scale),
                     .options = items,
                     .placeholder =
                         translator.text(MessageId::ToolbarGameMenu),
                     .open = menuOpen});

                ui.button(
                    translator.text(MessageId::ToolbarMenu),
                    {.id = widgets::kMenu});

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

                ui.spacer(kGrow);

                {
                    const auto palette = ui.panel(
                        {.width = fixedSize(paletteWidth),
                         .height = kGrow,
                         .id = widgets::kSidePanel});

                    ui.label(translator.text(MessageId::ToolbarBuild));

                    for (std::size_t first = 0; first < kBuildToolCount;
                         first += kPaletteColumns)
                    {
                        const auto row = ui.row({.width = kGrow});

                        for (std::size_t column = 0;
                             column < kPaletteColumns;
                             ++column)
                        {
                            const auto index = first + column;
                            const auto tool =
                                static_cast<BuildTool>(index);

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

                ui.button(
                    translator.text(pauseLabel(paused)),
                    {.id = widgets::kPauseResume,
                     .state = paused
                                  ? std::optional{ButtonState::Pressed}
                                  : std::nullopt});

                ui.spacer(kGrow);

                const auto zoom = std::to_string(camera.zoomLevel());
                const std::array<std::string_view, 1> zoomArgs{zoom};
                ui.label(
                    translator.formatted(
                        MessageId::ToolbarZoomLevel, zoomArgs));

                const auto counted = std::to_string(tick);
                const std::array<std::string_view, 1> tickArgs{counted};
                ui.label(
                    translator.formatted(
                        MessageId::ToolbarTick, tickArgs));

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

                const auto bank = std::to_string(funds);
                const std::array<std::string_view, 1> bankArgs{bank};
                ui.label(
                    translator.formatted(
                        MessageId::ToolbarMoney, bankArgs));
            }
        }

        return ui.finish();
    }

}
