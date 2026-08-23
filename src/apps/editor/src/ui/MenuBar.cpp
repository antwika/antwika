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

        struct MenuItemRow final
        {
            MenuItem item;
            std::string_view name;
            bool toggle;
        };

        constexpr std::array<MenuItemRow, enums::kCount<MenuItem>>
            kMenuItemRows{{
            {MenuItem::New, "New", false},
            {MenuItem::Save, "Save", false},
            {MenuItem::Load, "Load", false},
            {MenuItem::Settings, "Settings", false},
            {MenuItem::Quit, "Quit", false},
            {MenuItem::Undo, "Undo", false},
            {MenuItem::Redo, "Redo", false},
            {MenuItem::Grow, "Grow a block", false},
            {MenuItem::Keys, "Keys", false},
            {MenuItem::FreeLook, "Free look camera", true},
            {MenuItem::Grid, "Grid", true},
            {MenuItem::Marker, "Placement marker", true},
            {MenuItem::RuleLines, "Rule lines", true},
            {MenuItem::Lighting, "Lighting", true},
            {MenuItem::Sight, "Line of sight", true},
            {MenuItem::LowerSight, "Lower line-of-sight", true},
            {MenuItem::LowerLight, "Player lower point-light", true},
            {MenuItem::Follow, "Camera follows", true},
            {MenuItem::Corners, "Corners joined", true},
            {MenuItem::AboveHidden, "Hide above level", true}}};

        static_assert(enums::tagsInOrder(kMenuItemRows, &MenuItemRow::item));

        template <std::size_t Size>
        [[nodiscard]] constexpr std::array<std::string_view, Size>
            namesOfItems(const std::array<MenuItem, Size> &chosenItems)
        {
            std::array<std::string_view, Size> names{};

            for (std::size_t slotIndex = 0; slotIndex < Size; ++slotIndex)
            {
                names[slotIndex] =
                    enums::lookup(kMenuItemRows, chosenItems[slotIndex]).name;
            }

            return names;
        }

        constexpr auto kFileNames = namesOfItems(kFileItems);

        constexpr auto kEditNames = namesOfItems(kEditItems);

        constexpr auto kViewNames = namesOfItems(kViewItems);

        constexpr auto kSettingsNames = namesOfItems(kSettingsItems);

        struct MenuRow final
        {
            Menu menu;
            std::string_view name;
            widget::WidgetId widget;
            widget::WidgetId firstItemWidget;
            std::span<const MenuItem> items;
            std::span<const std::string_view> names;
        };

        constexpr std::array<MenuRow, enums::kCount<Menu>> kMenuRows{{
            {Menu::File,
             "File",
             widget::WidgetId{1},
             widget::WidgetId{kFirstItemWidget},
             kFileItems,
             kFileNames},
            {Menu::Edit,
             "Edit",
             widget::WidgetId{4},
             widget::WidgetId{8},
             kEditItems,
             kEditNames},
            {Menu::View,
             "View",
             widget::WidgetId{2},
             widget::WidgetId{kFirstItemWidget + kWidgetsPerMenu},
             kViewItems,
             kViewNames},
            {Menu::Settings,
             "Settings",
             widget::WidgetId{3},
             widget::WidgetId{kFirstItemWidget + (2 * kWidgetsPerMenu)},
             kSettingsItems,
             kSettingsNames}}};

        static_assert(enums::tagsInOrder(kMenuRows, &MenuRow::menu));
    }

    std::string_view menuName(const Menu menu)
    {
        return enums::lookup(kMenuRows, menu).name;
    }

    std::string_view itemName(const MenuItem item)
    {
        return enums::lookup(kMenuItemRows, item).name;
    }

    std::span<const MenuItem> itemsOf(const Menu menu)
    {
        return enums::lookup(kMenuRows, menu).items;
    }

    bool isToggle(const MenuItem item)
    {
        return enums::lookup(kMenuItemRows, item).toggle;
    }

    widget::WidgetId menuWidget(const Menu menu)
    {
        return enums::lookup(kMenuRows, menu).widget;
    }

    widget::WidgetId firstItemWidget(const Menu menu)
    {
        return enums::lookup(kMenuRows, menu).firstItemWidget;
    }

    std::span<const std::string_view> itemNamesOf(const Menu menu)
    {
        return enums::lookup(kMenuRows, menu).names;
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
