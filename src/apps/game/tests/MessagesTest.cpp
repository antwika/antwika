#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::game::MessageId;

        constexpr std::array<MessageId, 4> kSameInBoth{
            MessageId::ToolbarZoomLevel,
            MessageId::ToolbarTick,
            MessageId::MenuTitle,
            MessageId::ReadoutAmount,
        };

        struct GameMessageTraits final
        {
            using Messages = antwika::game::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Game, MessageSetCompletenessTest, GameMessageTraits);

}
