#include "antwika/editor/ui/MenuBar.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "antwika/editor/ui/WidgetCatalog.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::array kFileItems{
            MenuItem::New,
            MenuItem::Save,
            MenuItem::Load,
            MenuItem::Settings,
            MenuItem::Quit};

        constexpr std::array kEditItems{
            MenuItem::Undo, MenuItem::Redo, MenuItem::Grow};

        constexpr std::array kViewItems{
            MenuItem::FreeLook,
            MenuItem::Follow,
            MenuItem::Sight,
            MenuItem::LowerSight,
            MenuItem::LowerLight,
            MenuItem::Grid,
            MenuItem::Marker,
            MenuItem::RuleLines,
            MenuItem::AboveHidden};

        constexpr std::array kSettingsItems{
            MenuItem::Lighting,
            MenuItem::Corners,
            MenuItem::Keys};

        constexpr std::uint64_t kWidgetsPerMenu = 16;

        constexpr std::uint64_t kFirstItemWidget = 16;

        constexpr std::array<std::string_view, 5> kFileNames{
            "New", "Save", "Load", "Settings", "Quit"};

        constexpr std::array<std::string_view, 3> kEditNames{
            "Undo", "Redo", "Grow a block"};

        constexpr std::array<std::string_view, 9> kViewNames{
            "Free look camera",
            "Camera follows",
            "Line of sight",
            "Lower line-of-sight",
            "Player lower point-light",
            "Grid",
            "Placement marker",
            "Rule lines",
            "Hide above level"};

        constexpr std::array<std::string_view, 3> kSettingsNames{
            "Lighting", "Corners joined", "Keys"};

    }

    std::string_view menuName(const Menu menu)
    {
        switch (menu)
        {
        case Menu::File:
            return "File";
        case Menu::Edit:
            return "Edit";
        case Menu::View:
            return "View";
        case Menu::Settings:
            break;
        }

        return "Settings";
    }

    std::string_view itemName(const MenuItem item)
    {
        switch (item)
        {
        case MenuItem::New:
            return "New";
        case MenuItem::Save:
            return "Save";
        case MenuItem::Load:
            return "Load";
        case MenuItem::Settings:
            return "Settings";
        case MenuItem::Quit:
            return "Quit";
        case MenuItem::Undo:
            return "Undo";
        case MenuItem::Redo:
            return "Redo";
        case MenuItem::Grow:
            return "Grow a block";
        case MenuItem::Keys:
            return "Keys";
        case MenuItem::FreeLook:
            return "Free look camera";
        case MenuItem::Grid:
            return "Grid";
        case MenuItem::Marker:
            return "Placement marker";
        case MenuItem::RuleLines:
            return "Rule lines";
        case MenuItem::Lighting:
            return "Lighting";
        case MenuItem::Sight:
            return "Line of sight";
        case MenuItem::LowerSight:
            return "Lower line-of-sight";
        case MenuItem::LowerLight:
            return "Player lower point-light";
        case MenuItem::Follow:
            return "Camera follows";
        case MenuItem::AboveHidden:
            return "Hide above level";
        case MenuItem::Corners:
            break;
        }

        return "Corners joined";
    }

    std::span<const MenuItem> itemsOf(const Menu menu)
    {
        switch (menu)
        {
        case Menu::File:
            return kFileItems;
        case Menu::Edit:
            return kEditItems;
        case Menu::View:
            return kViewItems;
        case Menu::Settings:
            break;
        }

        return kSettingsItems;
    }

    bool isToggle(const MenuItem item)
    {
        switch (item)
        {
        case MenuItem::FreeLook:
        case MenuItem::Grid:
        case MenuItem::Marker:
        case MenuItem::RuleLines:
        case MenuItem::Lighting:
        case MenuItem::Sight:
        case MenuItem::LowerSight:
        case MenuItem::LowerLight:
        case MenuItem::Follow:
        case MenuItem::Corners:
        case MenuItem::AboveHidden:
            return true;
        default:
            break;
        }

        return false;
    }

    ui::WidgetId menuWidget(const Menu menu)
    {
        switch (menu)
        {
        case Menu::File:
            return ui::WidgetId{1};
        case Menu::Edit:
            return ui::WidgetId{4};
        case Menu::View:
            return ui::WidgetId{2};
        case Menu::Settings:
            break;
        }

        return ui::WidgetId{3};
    }

    ui::WidgetId firstItemWidget(const Menu menu)
    {
        switch (menu)
        {
        case Menu::File:
            return ui::WidgetId{kFirstItemWidget};
        case Menu::Edit:
            return ui::WidgetId{8};
        case Menu::View:
            return ui::WidgetId{kFirstItemWidget + kWidgetsPerMenu};
        case Menu::Settings:
            break;
        }

        return ui::WidgetId{
            kFirstItemWidget + (2 * kWidgetsPerMenu)};
    }

    std::span<const std::string_view> itemNamesOf(const Menu menu)
    {
        switch (menu)
        {
        case Menu::File:
            return kFileNames;
        case Menu::Edit:
            return kEditNames;
        case Menu::View:
            return kViewNames;
        case Menu::Settings:
            break;
        }

        return kSettingsNames;
    }

    std::optional<MenuItem> itemAt(
        const Menu menu, const std::size_t index)
    {
        const auto items = itemsOf(menu);

        if (index >= items.size())
        {
            return std::nullopt;
        }

        return items[index];
    }

}
