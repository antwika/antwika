#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include <cstddef>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/widget/WidgetId.hpp>

namespace antwika::editor
{

    enum class Menu : std::uint8_t
    {
        File,
        Edit,
        View,
        Settings,
    };

    [[nodiscard]] constexpr Menu getLastEnumerator(Menu) noexcept
    {
        return Menu::Settings;
    }

    enum class MenuItem : std::uint8_t
    {
        New,
        Save,
        Load,
        Settings,
        Quit,
        Undo,
        Redo,

        Grow,

        Keys,
        FreeLook,
        Grid,
        Marker,
        RuleLines,
        Lighting,
        Sight,
        LowerSight,
        LowerLight,
        Follow,
        Corners,

        AboveHidden,
    };

    [[nodiscard]] constexpr MenuItem getLastEnumerator(MenuItem) noexcept
    {
        return MenuItem::AboveHidden;
    }

    inline constexpr std::array<Menu, 3> kBarMenus{
        Menu::File, Menu::Edit, Menu::View};

    [[nodiscard]] std::string_view getMenuName(Menu menu);

    [[nodiscard]] std::string_view getItemName(MenuItem item);

    [[nodiscard]] std::span<const MenuItem> itemsOf(Menu menu);

    [[nodiscard]] bool isToggle(MenuItem item);

    [[nodiscard]] widget::WidgetId getMenuWidget(Menu menu);

    [[nodiscard]] widget::WidgetId getFirstItemWidget(Menu menu);

    [[nodiscard]] std::span<const std::string_view> itemNamesOf(
        Menu menu);

    [[nodiscard]] std::optional<MenuItem> itemAt(
        Menu menu, std::size_t index);

}
