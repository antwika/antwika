#include <span>

#include <antwika/i18n/conformance/MessageSetCompleteness.hpp>

#include "antwika/tower_defence/MessageId.hpp"
#include "antwika/tower_defence/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::tower_defence::MessageId;

        /**
         * @brief This module's ids, for the shared completeness suite.
         */
        struct TowerDefenceMessageTraits
        {
            using Messages = antwika::tower_defence::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return {};
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TowerDefence, MessageSetCompleteness, TowerDefenceMessageTraits);

} // namespace antwika::i18n::conformance
