#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/MessageId.hpp"

namespace antwika::game
{

    enum class MenuItem : std::uint8_t
    {
        NewGame = 0,
        SaveGame,
        LoadGame,
        MainMenu,
        WorldMap,
    };

    [[nodiscard]] constexpr MenuItem enumBound(MenuItem) noexcept
    {
        return MenuItem::WorldMap;
    }

    inline constexpr std::size_t kMenuItemCount =
        antwika::enums::kCount<MenuItem>;

    [[nodiscard]] constexpr std::size_t menuItemIndex(
        const MenuItem item) noexcept
    {
        return antwika::enums::index(item);
    }

    [[nodiscard]] constexpr MessageId menuItemLabel(MenuItem item) noexcept
    {
        constexpr std::array<MessageId, kMenuItemCount> labels{
            MessageId::MenuItemNewGame,
            MessageId::MenuItemSaveGame,
            MessageId::MenuItemLoadGame,
            MessageId::MenuItemMainMenu,
            MessageId::MenuItemWorldMap};

        return antwika::enums::pick(labels, item);
    }

    static_assert(
        []
        {
            for (std::size_t left = 0; left < kMenuItemCount; ++left)
            {
                for (std::size_t right = left + 1; right < kMenuItemCount;
                     ++right)
                {
                    if (menuItemLabel(static_cast<MenuItem>(left))
                        == menuItemLabel(static_cast<MenuItem>(right)))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "every menu item needs a caption of its own");

}
