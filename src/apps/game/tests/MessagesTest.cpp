#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompleteness.hpp>

#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::game::MessageId;

        // The ids whose two texts genuinely read the same in both.
        // A notation, a loanword or a noise an animal makes.
        // Written out rather than tolerated wherever it happens.
        // A forgotten Swedish entry looks exactly like one of these.
        // So the only way to be excused is to be named here.
        constexpr std::array<MessageId, 4> kSameInBoth{
            MessageId::ToolbarZoomLevel,
            MessageId::ToolbarTick,
            MessageId::MenuTitle,
            MessageId::ReadoutAmount,
        };

        /**
         * @brief This module's ids, for the shared completeness suite.
         */
        struct GameMessageTraits
        {
            using Messages = antwika::game::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Game, MessageSetCompleteness, GameMessageTraits);

} // namespace antwika::i18n::conformance
