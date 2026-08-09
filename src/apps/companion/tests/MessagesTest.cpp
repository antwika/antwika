#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/companion/MessageId.hpp"
#include "antwika/companion/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::companion::MessageId;

        constexpr std::array<MessageId, 5> kSameInBoth{
            MessageId::Hunger,
            MessageId::SayLaLaLa,
            MessageId::SayZzz,
            MessageId::SayWheee,
            MessageId::Day,
        };

        struct CompanionMessageTraits final
        {
            using Messages = antwika::companion::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Companion, MessageSetCompletenessTest, CompanionMessageTraits);

}
