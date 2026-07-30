#pragma once

#include <string_view>

#include "antwika/game/MenuState.hpp"

namespace antwika::game
{

    /**
     * @brief Every string the menu puts in front of a person.
     *
     * One struct rather than literals spread through the layout, so
     * translating the menu is a matter of producing a different one of
     * these -- see labelsFor(). When antwika::i18n lands, that function
     * becomes a catalogue lookup and nothing else here changes.
     *
     * The defaults are English, so MenuLabels{} is the English
     * catalogue and no second copy of it exists.
     *
     * Views rather than strings, because every label here is a literal
     * and a catalogue that allocates eight times is eight allocations
     * per described frame.
     * Whatever hands these out owns the characters, which is what a
     * catalogue does anyway.
     */
    struct MenuLabels
    {
        /**
         * @brief The heading above the entries.
         */
        std::string_view title = "Antwika";

        /**
         * @brief Start a game from the beginning.
         */
        std::string_view playGame = "Play game";

        /**
         * @brief Play a recorded session back.
         */
        std::string_view loadReplay = "Load replay";

        /**
         * @brief Write the session so far out.
         */
        std::string_view saveReplay = "Save replay";

        /**
         * @brief Go back to the game already under way.
         */
        std::string_view resumeGame = "Resume game";

        /**
         * @brief The heading above the language selector.
         */
        std::string_view language = "Language";

        /**
         * @brief What English is called, in English.
         */
        std::string_view english = "English";

        /**
         * @brief What Swedish is called, in Swedish.
         */
        std::string_view swedish = "Svenska";

        /**
         * @brief Compare two catalogues.
         * @param other The catalogue to compare against.
         * @return True when every string matches.
         */
        [[nodiscard]] constexpr bool operator==(
            const MenuLabels &other) const = default;
    };

    /**
     * @brief Get the catalogue a language is written in.
     *
     * The one place a language turns into words, and the one line that
     * changes when antwika::i18n takes the catalogues over.
     *
     * @param language The language to write the menu in.
     * @return That language's strings; English is MenuLabels{}.
     */
    [[nodiscard]] MenuLabels labelsFor(MenuLanguage language) noexcept;

    /**
     * @brief Get what an entry is called.
     * @param labels The catalogue to read from.
     * @param entry The entry to name.
     * @return The entry's label, viewed in the catalogue.
     */
    [[nodiscard]] std::string_view labelFor(
        const MenuLabels &labels, MenuEntry entry) noexcept;

    /**
     * @brief Get what a language is called.
     *
     * In its own language rather than the one on screen, which is what
     * makes the selector readable to somebody who cannot read the
     * language it is currently in.
     *
     * @param labels The catalogue to read from.
     * @param language The language to name.
     * @return The language's name, viewed in the catalogue.
     */
    [[nodiscard]] std::string_view labelFor(
        const MenuLabels &labels, MenuLanguage language) noexcept;

} // namespace antwika::game
