#include "antwika/game/MenuLabels.hpp"

#include <array>

namespace antwika::game
{

    namespace
    {
        // The bitmap font antwika::gfx draws with covers ASCII only.
        // So the Swedish strings are transliterated rather than spelled.
        // A glyph outside that range measures one cell and draws none.
        // The button would then be wider than the label inside it.
        // Restoring the diacritics is a font question, not a menu one.
        [[nodiscard]] MenuLabels swedishLabels()
        {
            return MenuLabels{
                .title = "Antwika",
                .playGame = "Spela spel",
                .loadReplay = "Ladda repris",
                .saveReplay = "Spara repris",
                .resumeGame = "Ateruppta spel",
                .language = "Sprak",
                .english = "English",
                .swedish = "Svenska"};
        }
    } // namespace

    MenuLabels labelsFor(MenuLanguage language)
    {
        const std::array<MenuLabels, kMenuLanguageCount> catalogue{
            MenuLabels{}, swedishLabels()};

        return catalogue
            [menuLanguageIndex(language) % kMenuLanguageCount];
    }

    const std::string &labelFor(
        const MenuLabels &labels, MenuEntry entry)
    {
        // A table rather than a switch, as Direction.hpp explains.
        // There is then no out-of-range arm for a coverage gate.
        constexpr std::array<std::string MenuLabels::*, kMenuEntryCount>
            fields{
                &MenuLabels::playGame,
                &MenuLabels::loadReplay,
                &MenuLabels::saveReplay,
                &MenuLabels::resumeGame};

        return labels.*fields[menuEntryIndex(entry) % kMenuEntryCount];
    }

    const std::string &labelFor(
        const MenuLabels &labels, MenuLanguage language)
    {
        constexpr std::
            array<std::string MenuLabels::*, kMenuLanguageCount>
                fields{&MenuLabels::english, &MenuLabels::swedish};

        return labels
            .*fields[menuLanguageIndex(language) % kMenuLanguageCount];
    }

} // namespace antwika::game
