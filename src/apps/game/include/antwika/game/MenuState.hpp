#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace antwika::game
{

    /**
     * @brief The language the menu is written in.
     *
     * Values are contiguous from zero, so a language can index a table.
     */
    enum class MenuLanguage : std::uint8_t
    {
        English = 0,
        Swedish,
    };

    /**
     * @brief How many languages the menu offers.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kMenuLanguageCount =
        static_cast<std::size_t>(MenuLanguage::Swedish) + 1;

    /**
     * @brief Get a language's index, for addressing a per-language
     * table.
     * @param language The language to index.
     * @return The index, always below kMenuLanguageCount for a named
     * language.
     */
    [[nodiscard]] constexpr std::size_t menuLanguageIndex(
        MenuLanguage language) noexcept
    {
        return static_cast<std::size_t>(language);
    }

    /**
     * @brief Every language, in the order the selector lists them.
     */
    inline constexpr std::array<MenuLanguage, kMenuLanguageCount>
        kMenuLanguages{MenuLanguage::English, MenuLanguage::Swedish};

    /**
     * @brief One thing the menu can be asked to do.
     *
     * Values are contiguous from zero, so an entry can index a table,
     * and ResumeGame is last because that is the one entriesFor() drops
     * before a game has begun.
     */
    enum class MenuEntry : std::uint8_t
    {
        PlayGame = 0,
        LoadReplay,
        SaveReplay,
        ResumeGame,
    };

    /**
     * @brief How many entries the menu has at most.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kMenuEntryCount =
        static_cast<std::size_t>(MenuEntry::ResumeGame) + 1;

    /**
     * @brief Get an entry's index, for addressing a per-entry table.
     * @param entry The entry to index.
     * @return The index, always below kMenuEntryCount for a named entry.
     */
    [[nodiscard]] constexpr std::size_t menuEntryIndex(
        MenuEntry entry) noexcept
    {
        return static_cast<std::size_t>(entry);
    }

    /**
     * @brief Every entry, in the order the menu lists them.
     */
    inline constexpr std::array<MenuEntry, kMenuEntryCount> kMenuEntries{
        MenuEntry::PlayGame,
        MenuEntry::LoadReplay,
        MenuEntry::SaveReplay,
        MenuEntry::ResumeGame};

    /**
     * @brief Get the entries the menu shows.
     *
     * A prefix of kMenuEntries rather than a list built per call, so
     * "which entries are shown" is one fact both the layout and a
     * hit-test read -- the same reason life::layoutFor() is shared.
     *
     * @param gameBegun Whether a game has been started yet.
     * @return Every entry, or every entry but ResumeGame: there is
     * nothing to resume until something has begun.
     */
    [[nodiscard]] constexpr std::span<const MenuEntry> entriesFor(
        bool gameBegun) noexcept
    {
        return std::span<const MenuEntry>{kMenuEntries}.first(
            gameBegun ? kMenuEntryCount : kMenuEntryCount - 1);
    }

    /**
     * @brief Check whether activating an entry puts the menu away.
     *
     * Playing, loading and resuming all hand the window back to the
     * game, so the menu that asked for them has nothing left to show.
     * Saving does not: it leaves the session exactly where it was, and
     * closing on it would be a different answer to a question nobody
     * asked -- see the note in MenuSink.
     *
     * @param entry The entry that was activated.
     * @return True when the menu should close on it.
     */
    [[nodiscard]] constexpr bool leavesMenu(MenuEntry entry) noexcept
    {
        return entry != MenuEntry::SaveReplay;
    }

    /**
     * @brief The menu's whole state, as a plain value.
     *
     * A value rather than an object with behaviour, so a test can write
     * the situation it means and compare the one it got. What changes it
     * is MenuSink, from recorded input alone, which is what keeps a
     * replay reaching the same menu.
     *
     * It is not persisted and defines no event: what a recording holds
     * is the F10 press and the click, and every field below is worked
     * out from those again -- see MenuSink.
     */
    struct MenuState
    {
        /**
         * @brief Whether the menu is up.
         */
        bool open = false;

        /**
         * @brief Whether a game has been started.
         *
         * What decides the entry set, and set by activating PlayGame.
         */
        bool gameBegun = false;

        /**
         * @brief The language every label is written in.
         */
        MenuLanguage language = MenuLanguage::English;

        /**
         * @brief The entry activated during the current tick, if any.
         *
         * The intent the application acts on: this library opens no file
         * dialog, so "load" and "save" are reported rather than done.
         * Cleared when the next tick's first event arrives, so a sink
         * registered after MenuSink still sees it inside the tick it
         * happened in.
         */
        std::optional<MenuEntry> activated{};

        /**
         * @brief Compare two menus.
         * @param other The state to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const MenuState &other) const
            = default;
    };

} // namespace antwika::game
