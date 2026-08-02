#include "antwika/sudoku/Messages.hpp"

#include <array>
#include <cstddef>
#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>

#include "antwika/sudoku/MessageId.hpp"

namespace antwika::sudoku
{

    namespace
    {

        using i18n::Catalogue;
        using i18n::CatalogueEntry;
        using i18n::Locale;
        using i18n::MessageName;

        constexpr std::array<MessageName<MessageId>, 8> kNames{{
            {MessageId::Title, "Title"},
            {MessageId::SolveButton, "SolveButton"},
            {MessageId::Hint, "Hint"},
            {MessageId::Solved, "Solved"},
            {MessageId::Complete, "Complete"},
            {MessageId::NoSolution, "NoSolution"},
            {MessageId::LimitExceeded, "LimitExceeded"},
            {MessageId::GivenLocked, "GivenLocked"},
        }};

        static_assert(
            kNames.size() == static_cast<std::size_t>(MessageId::Count),
            "every MessageId must appear in kNames exactly once");

        // Both arrays list every id, in the same order.
        // MessagesTest asserts they cover exactly kNames.
        // That assertion is the point of keying by id.
        // A forgotten Swedish entry is a red build, not a wrong label.
        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kEnglishEntries{{
                {MessageId::Title, "Sudoku"},
                {MessageId::SolveButton, "Solve"},
                {MessageId::Hint,
                 "Pick a square, then type 1-9. Backspace clears it."},
                {MessageId::Solved, "Solved."},
                {MessageId::Complete, "Finished. Every square agrees."},
                {MessageId::NoSolution, "No solution exists from here."},
                {MessageId::LimitExceeded, "Solver step limit exceeded."},
                {MessageId::GivenLocked, "That square is a clue."},
            }};

        constexpr std::array<CatalogueEntry<MessageId>, kNames.size()>
            kSwedishEntries{{
                {MessageId::Title, "Sudoku"},
                {MessageId::SolveButton, "Lös"},
                {MessageId::Hint,
                 "Välj en ruta och skriv 1-9. Backsteg tömmer den."},
                {MessageId::Solved, "Löst."},
                {MessageId::Complete, "Klart. Alla rutor stämmer."},
                {MessageId::NoSolution, "Det finns ingen lösning härifrån."},
                {MessageId::LimitExceeded, "Lösarens stegtak överskreds."},
                {MessageId::GivenLocked, "Den rutan är en ledtråd."},
            }};

        constexpr Catalogue<MessageId> kEnglishCatalogue{
            Locale::English, kEnglishEntries};

        constexpr Catalogue<MessageId> kSwedishCatalogue{
            Locale::Swedish, kSwedishEntries};

    } // namespace

    std::span<const i18n::MessageName<MessageId>>
        Messages::names() noexcept
    {
        return kNames;
    }

    const i18n::Catalogue<MessageId> &Messages::catalogueFor(
        i18n::Locale locale) noexcept
    {
        switch (locale)
        {
        case Locale::English:
            return kEnglishCatalogue;
        case Locale::Swedish:
            return kSwedishCatalogue;
        }

        return catalogueFor(i18n::kDefaultLocale);
    }

} // namespace antwika::sudoku
