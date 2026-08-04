#include "antwika/sudoku/Messages.hpp"

#include <antwika/i18n/MessageTable.hpp>

#include "antwika/sudoku/MessageId.hpp"

namespace antwika::sudoku
{

    // Every array lists every id, in the same order.
    // A forgotten entry is a value-initialised hole, not a short array.
    // isComplete() below is what sees that hole.
    // A missing Swedish string is a red build, not a wrong label.
    constexpr i18n::MessageTable<MessageId> kMessageTable{
        .names{{
            {MessageId::Title, "Title"},
            {MessageId::SolveButton, "SolveButton"},
            {MessageId::Hint, "Hint"},
            {MessageId::Solved, "Solved"},
            {MessageId::Complete, "Complete"},
            {MessageId::NoSolution, "NoSolution"},
            {MessageId::LimitExceeded, "LimitExceeded"},
            {MessageId::GivenLocked, "GivenLocked"},
        }},
        .english{{
            {MessageId::Title, "Sudoku"},
            {MessageId::SolveButton, "Solve"},
            {MessageId::Hint,
             "Pick a square, then type 1-9. Backspace clears it."},
            {MessageId::Solved, "Solved."},
            {MessageId::Complete, "Finished. Every square agrees."},
            {MessageId::NoSolution, "No solution exists from here."},
            {MessageId::LimitExceeded, "Solver step limit exceeded."},
            {MessageId::GivenLocked, "That square is a clue."},
        }},
        .swedish{{
            {MessageId::Title, "Sudoku"},
            {MessageId::SolveButton, "Lös"},
            {MessageId::Hint,
             "Välj en ruta och skriv 1-9. Backsteg tömmer den."},
            {MessageId::Solved, "Löst."},
            {MessageId::Complete, "Klart. Alla rutor stämmer."},
            {MessageId::NoSolution, "Det finns ingen lösning härifrån."},
            {MessageId::LimitExceeded, "Lösarens stegtak överskreds."},
            {MessageId::GivenLocked, "Den rutan är en ledtråd."},
        }},
    };

    static_assert(
        i18n::isComplete(kMessageTable),
        "every MessageId needs a name and text in both locales");

} // namespace antwika::sudoku
