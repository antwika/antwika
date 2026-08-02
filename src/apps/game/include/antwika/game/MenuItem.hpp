#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/i18n/MessageId.hpp>

namespace antwika::game
{

    using antwika::i18n::MessageId;

    /**
     * @brief What the toolbar's game menu offers.
     *
     * The list a player reads, in the order it is read in. It is not
     * persisted anywhere and never will be: a recording holds the click
     * that opened the list and the click that landed on an item, and
     * which item that was is worked out again from the layout -- see
     * UiSink.
     *
     * Values are contiguous from zero, so an item can index a table and
     * an option index coming back out of antwika::ui is one of these.
     */
    enum class MenuItem : std::uint8_t
    {
        NewGame = 0,
        SaveGame,
        LoadGame,
        MainMenu,
        WorldMap,
    };

    /**
     * @brief How many items the menu lists.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kMenuItemCount =
        static_cast<std::size_t>(MenuItem::WorldMap) + 1;

    /**
     * @brief Get an item's index, for addressing a per-item table.
     * @param item The item to index.
     * @return The index, always below kMenuItemCount for a named item.
     */
    [[nodiscard]] constexpr std::size_t menuItemIndex(
        MenuItem item) noexcept
    {
        return static_cast<std::size_t>(item);
    }

    /**
     * @brief What each item is labelled.
     *
     * **A MessageId rather than the English words**, for the reason
     * toolLabel() gives: a label is read by a person, so it goes through
     * antwika::i18n like every other caption.
     *
     * @param item The item to name.
     * @return The label's id, in MenuItem order.
     */
    [[nodiscard]] constexpr MessageId menuItemLabel(MenuItem item) noexcept
    {
        constexpr std::array<MessageId, kMenuItemCount> labels{
            MessageId::GameMenuItemNewGame,
            MessageId::GameMenuItemSaveGame,
            MessageId::GameMenuItemLoadGame,
            MessageId::GameMenuItemMainMenu,
            MessageId::GameMenuItemWorldMap};

        return labels[menuItemIndex(item) % kMenuItemCount];
    }

    // Two items sharing a caption would be two entries reading the same.
    // The table above is where that can happen, so it is checked here.
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

} // namespace antwika::game
