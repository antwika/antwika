#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/tower_defence/MessageId.hpp"
#include "antwika/tower_defence/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::tower_defence::MessageId;

        struct TowerDefenceMessageTraits final
        {
            using Messages = antwika::tower_defence::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return {};
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TowerDefence, MessageSetCompletenessTest, TowerDefenceMessageTraits);

}
