#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/sudoku/MessageId.hpp"
#include "antwika/sudoku/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::sudoku::MessageId;

        constexpr std::array<MessageId, 1> kSameInBoth{
            MessageId::Title,
        };

        struct SudokuMessageTraits final
        {
            using Messages = antwika::sudoku::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sudoku, MessageSetCompletenessTest, SudokuMessageTraits);

}
