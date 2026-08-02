#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompleteness.hpp>

#include "antwika/sudoku/MessageId.hpp"
#include "antwika/sudoku/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::sudoku::MessageId;

        // The ids whose two texts genuinely read the same in both.
        // A notation, a loanword or a noise an animal makes.
        // Written out rather than tolerated wherever it happens.
        // A forgotten Swedish entry looks exactly like one of these.
        // So the only way to be excused is to be named here.
        constexpr std::array<MessageId, 1> kSameInBoth{
            MessageId::Title,
        };

        /**
         * @brief This module's ids, for the shared completeness suite.
         */
        struct SudokuMessageTraits
        {
            using Messages = antwika::sudoku::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Sudoku, MessageSetCompleteness, SudokuMessageTraits);

} // namespace antwika::i18n::conformance
