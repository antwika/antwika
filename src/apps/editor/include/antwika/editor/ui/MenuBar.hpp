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
        Game,
        View,
    };

    [[nodiscard]] constexpr Menu getLastEnumerator(Menu) noexcept
    {
        return Menu::View;
    }

    enum class MenuItem : std::uint8_t
    {
        New,
        Save,
        Load,
        Keys,
        Quit,
        Undo,
        Redo,

        Grow,

        GameLighting,
        Corners,

        FreeLook,
        Grid,
        Marker,
        RuleLines,
        EditorLighting,
        Sight,
        LowerSight,
        LowerLight,
        Follow,

        AboveHidden,
    };

    [[nodiscard]] constexpr MenuItem getLastEnumerator(MenuItem) noexcept
    {
        return MenuItem::AboveHidden;
    }

    inline constexpr std::array<Menu, enums::kCount<Menu>> kEveryMenu =
        enums::kAll<Menu>;

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
