#include "antwika/editor/ui/MenuBar.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::array kFileItems{
            MenuItem::New,
            MenuItem::Save,
            MenuItem::Load,
            MenuItem::Keys,
            MenuItem::Quit};

        constexpr std::array kEditItems{
            MenuItem::Undo, MenuItem::Redo, MenuItem::Grow};

        constexpr std::array kGameItems{
            MenuItem::GameLighting, MenuItem::Corners};

        constexpr std::array kViewItems{
            MenuItem::EditorLighting,
            MenuItem::FreeLook,
            MenuItem::Follow,
            MenuItem::Sight,
            MenuItem::LowerSight,
            MenuItem::LowerLight,
            MenuItem::Grid,
            MenuItem::Marker,
            MenuItem::RuleLines,
            MenuItem::AboveHidden};

        static_assert(kFileItems.size() <= kMaxItemsPerMenu);

        static_assert(kEditItems.size() <= kMaxItemsPerMenu);

        static_assert(kGameItems.size() <= kMaxItemsPerMenu);

        static_assert(kViewItems.size() <= kMaxItemsPerMenu);

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
            {MenuItem::Keys, "Keys", false},
            {MenuItem::Quit, "Quit", false},
            {MenuItem::Undo, "Undo", false},
            {MenuItem::Redo, "Redo", false},
            {MenuItem::Grow, "Grow a block", false},
            {MenuItem::GameLighting, "Lighting", true},
            {MenuItem::Corners, "Corners joined", true},
            {MenuItem::FreeLook, "Free look camera", true},
            {MenuItem::Grid, "Grid", true},
            {MenuItem::Marker, "Placement marker", true},
            {MenuItem::RuleLines, "Rule lines", true},
            {MenuItem::EditorLighting, "Lighting", true},
            {MenuItem::Sight, "Line of sight", true},
            {MenuItem::LowerSight, "Lower line-of-sight", true},
            {MenuItem::LowerLight, "Player lower point-light", true},
            {MenuItem::Follow, "Camera follows", true},
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

        constexpr auto kGameNames = namesOfItems(kGameItems);

        constexpr auto kViewNames = namesOfItems(kViewItems);

        struct MenuRow final
        {
            Menu menu;
            std::string_view name;
            std::span<const MenuItem> items;
            std::span<const std::string_view> names;
        };

        constexpr std::array<MenuRow, enums::kCount<Menu>> kMenuRows{{
            {Menu::File, "File", kFileItems, kFileNames},
            {Menu::Edit, "Edit", kEditItems, kEditNames},
            {Menu::Game, "Game", kGameItems, kGameNames},
            {Menu::View, "View", kViewItems, kViewNames}}};

        static_assert(enums::tagsInOrder(kMenuRows, &MenuRow::menu));
    }

    std::string_view getMenuName(const Menu menu)
    {
        return enums::lookup(kMenuRows, menu).name;
    }

    std::string_view getItemName(const MenuItem item)
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

    widget::WidgetId getMenuWidget(const Menu menu)
    {
        return getWidgetAfter(
            kFirstMenuWidget, static_cast<std::uint64_t>(menu));
    }

    widget::WidgetId getFirstItemWidget(const Menu menu)
    {
        return getWidgetAfter(
            kFirstMenuItemWidget,
            static_cast<std::uint64_t>(menu) * kMaxItemsPerMenu);
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
