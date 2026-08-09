#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {
        struct I18nMessageTraits final
        {
            using Messages = antwika::i18n::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return {};
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        I18n, MessageSetCompletenessTest, I18nMessageTraits);

}
