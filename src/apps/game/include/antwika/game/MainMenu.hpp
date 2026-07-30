#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/MenuState.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the menu's widgets are called.
     *
     * Symbolic names rather than where a widget ended up in the layout,
     * for the reason game::widgets gives: the entry list changes shape
     * the moment a game has begun, so a declaration index would name a
     * different entry depending on the state it was resolved against.
     *
     * None of these ever reaches a replay: what is recorded is the
     * click, and which entry it hit is worked out again -- see MenuSink.
     *
     * Numbered above the toolbar's ids so that a reader comparing the
     * two files cannot mistake one bar for the other, though nothing
     * requires it: the two are described into separate ui::Contexts, and
     * an id only has to be distinct within one frame.
     */
    namespace menuWidgets
    {
        /**
         * @brief Start a game from the beginning.
         */
        inline constexpr WidgetId kPlayGame{101};

        /**
         * @brief Play a recorded session back.
         */
        inline constexpr WidgetId kLoadReplay{102};

        /**
         * @brief Write the session so far out.
         */
        inline constexpr WidgetId kSaveReplay{103};

        /**
         * @brief Go back to the game already under way.
         */
        inline constexpr WidgetId kResumeGame{104};

        /**
         * @brief Write the menu in English.
         */
        inline constexpr WidgetId kEnglish{105};

        /**
         * @brief Write the menu in Swedish.
         */
        inline constexpr WidgetId kSwedish{106};
    } // namespace menuWidgets

    static_assert(
        antwika::ui::assertDistinct(
            menuWidgets::kPlayGame,
            menuWidgets::kLoadReplay,
            menuWidgets::kSaveReplay,
            menuWidgets::kResumeGame,
            menuWidgets::kEnglish,
            menuWidgets::kSwedish),
        "every menu widget needs its own id");

    /**
     * @brief Each entry's widget, in kMenuEntries order.
     */
    inline constexpr std::array<WidgetId, kMenuEntryCount> kEntryWidgets{
        menuWidgets::kPlayGame,
        menuWidgets::kLoadReplay,
        menuWidgets::kSaveReplay,
        menuWidgets::kResumeGame};

    /**
     * @brief Each language's widget, in kMenuLanguages order.
     */
    inline constexpr std::array<WidgetId, kMenuLanguageCount>
        kLanguageWidgets{menuWidgets::kEnglish, menuWidgets::kSwedish};

    /**
     * @brief Get the widget an entry is drawn as.
     * @param entry The entry to name.
     * @return That entry's widget id.
     */
    [[nodiscard]] constexpr WidgetId widgetFor(MenuEntry entry) noexcept
    {
        return kEntryWidgets[menuEntryIndex(entry) % kMenuEntryCount];
    }

    /**
     * @brief Get the widget a language is drawn as.
     * @param language The language to name.
     * @return That language's widget id.
     */
    [[nodiscard]] constexpr WidgetId widgetForLanguage(
        MenuLanguage language) noexcept
    {
        return kLanguageWidgets
            [menuLanguageIndex(language) % kMenuLanguageCount];
    }

    /**
     * @brief Get the entry a widget stands for.
     * @param id The widget a press landed on.
     * @return The entry, or nullopt when the id is not an entry's.
     */
    [[nodiscard]] constexpr std::optional<MenuEntry> entryFor(
        WidgetId id) noexcept
    {
        for (std::size_t index = 0; index < kMenuEntryCount; ++index)
        {
            if (kEntryWidgets[index] == id)
            {
                return kMenuEntries[index];
            }
        }

        return std::nullopt;
    }

    /**
     * @brief Get the language a widget stands for.
     * @param id The widget a press landed on.
     * @return The language, or nullopt when the id is not a language's.
     */
    [[nodiscard]] constexpr std::optional<MenuLanguage> languageFor(
        WidgetId id) noexcept
    {
        for (std::size_t index = 0; index < kMenuLanguageCount; ++index)
        {
            if (kLanguageWidgets[index] == id)
            {
                return kMenuLanguages[index];
            }
        }

        return std::nullopt;
    }

    /**
     * @brief The menu drawn over the whole window.
     *
     * A pure function of the canvas, the pointer and the state, so the
     * same three always produce the same picture and the same answer
     * about what the pointer is on -- game::Toolbar's shape, and for the
     * same reason.
     *
     * It fills the canvas rather than a box in the middle of it, because
     * a modal has to cover what it is over: GridSink skips whatever the
     * overlay reports as covered, so a menu covering only its own pixels
     * would let a click through beside it and lay a tile behind the
     * menu.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports, because a hit-test
     * is a function of the layout and the layout is a function of the
     * canvas -- see UiCanvas.hpp.
     */
    class MainMenu final
    {
    public:
        /**
         * @brief Describe the menu for one tick.
         * @param canvas The area the UI is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @param state Which entries to show, and which language to
         * write them in.
         * @return The drawing commands and what the pointer did.
         */
        [[nodiscard]] Frame describe(
            Size canvas, Pointer pointer, const MenuState &state) const;
    };

} // namespace antwika::game
