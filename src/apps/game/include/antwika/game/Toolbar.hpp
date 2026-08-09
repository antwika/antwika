#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/MenuItem.hpp"
#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace widgets
    {
        inline constexpr WidgetId kZoomOut{1};

        inline constexpr WidgetId kZoomIn{2};

        inline constexpr WidgetId kResetView{3};

        inline constexpr WidgetId kPauseResume{4};

        inline constexpr WidgetId kFirstTool{5};

        inline constexpr WidgetId kMenu{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstTool) + kBuildToolCount)};

        inline constexpr WidgetId kGameMenu{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kMenu) + 1)};

        inline constexpr WidgetId kFirstMenuItem{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kMenu) + 2)};

        inline constexpr WidgetId kTopBar{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstMenuItem)
                + kMenuItemCount)};

        inline constexpr WidgetId kSidePanel{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kTopBar) + 1)};

        inline constexpr WidgetId kBottomBar{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kTopBar) + 2)};

        inline constexpr WidgetId kViewMenu{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kBottomBar) + 1)};

        inline constexpr WidgetId kFirstViewItem{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kViewMenu) + 1)};

        [[nodiscard]] constexpr WidgetId viewWidget(MapView view) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstViewItem)
                + mapViewIndex(view));
        }

        [[nodiscard]] constexpr WidgetId toolWidget(BuildTool tool) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstTool)
                + buildToolIndex(tool));
        }

        [[nodiscard]] constexpr WidgetId menuItemWidget(
            MenuItem item) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstMenuItem)
                + menuItemIndex(item));
        }
    }

    [[nodiscard]] constexpr MessageId toolLabel(BuildTool tool) noexcept
    {
        constexpr std::array<MessageId, kBuildToolCount> labels{
            MessageId::ToolRoad,
            MessageId::ToolHouse,
            MessageId::ToolFarm,
            MessageId::ToolClayPit,
            MessageId::ToolWorkshop,
            MessageId::ToolStorage,
            MessageId::ToolMarket,
            MessageId::ToolWell,
            MessageId::ToolDoctor,
            MessageId::ToolFireStation,
            MessageId::ToolEngineerPost,
            MessageId::ToolRaze};

        return antwika::enums::pick(labels, tool);
    }

    static_assert(
        antwika::ui::assertDistinct(
            widgets::kZoomOut,
            widgets::kZoomIn,
            widgets::kResetView,
            widgets::kPauseResume,
            widgets::toolWidget(BuildTool::Road),
            widgets::toolWidget(BuildTool::House),
            widgets::toolWidget(BuildTool::Farm),
            widgets::toolWidget(BuildTool::ClayPit),
            widgets::toolWidget(BuildTool::Workshop),
            widgets::toolWidget(BuildTool::Storage),
            widgets::toolWidget(BuildTool::Market),
            widgets::toolWidget(BuildTool::Well),
            widgets::toolWidget(BuildTool::Doctor),
            widgets::toolWidget(BuildTool::FireStation),
            widgets::toolWidget(BuildTool::EngineerPost),
            widgets::toolWidget(BuildTool::Raze),
            widgets::kMenu,
            widgets::kGameMenu,
            widgets::menuItemWidget(MenuItem::NewGame),
            widgets::menuItemWidget(MenuItem::SaveGame),
            widgets::menuItemWidget(MenuItem::LoadGame),
            widgets::menuItemWidget(MenuItem::MainMenu),
            widgets::menuItemWidget(MenuItem::WorldMap),
            widgets::kTopBar,
            widgets::kSidePanel,
            widgets::kBottomBar),
        "every toolbar widget needs its own id");

    static_assert(
        []
        {
            for (std::size_t left = 0; left < kBuildToolCount; ++left)
            {
                for (std::size_t right = left + 1; right < kBuildToolCount;
                     ++right)
                {
                    if (toolLabel(static_cast<BuildTool>(left))
                        == toolLabel(static_cast<BuildTool>(right)))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "every tool needs a caption of its own");

    [[nodiscard]] constexpr MessageId pauseLabel(bool paused) noexcept
    {
        return paused ? MessageId::ToolbarResume
                      : MessageId::ToolbarPause;
    }

    class Toolbar final
    {
    public:
        explicit Toolbar(const Translator &translator);

        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Camera &camera,
            std::optional<BuildTool> selected = BuildTool::Road,
            bool paused = false,
            antwika::time::Tick tick = 0,
            CityRatings ratings = {},
            bool menuOpen = false,
            MapView view = MapView::Normal,
            bool viewOpen = false,
            std::int64_t funds = kStartingMoney) const;

    private:
        const Translator &translator;
    };

}
