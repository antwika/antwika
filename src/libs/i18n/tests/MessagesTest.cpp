#include <span>

#include <antwika/i18n/conformance/MessageSetCompleteness.hpp>

#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {
        /**
         * @brief The library's own two ids, for the shared suite.
         */
        struct I18nMessageTraits
        {
            using Messages = antwika::i18n::Messages;

            // A language's name is a translation like any other.
            // So neither of these two is excused from being one.
            static std::span<const MessageId> sameInBothLocales()
            {
                return {};
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        I18n, MessageSetCompleteness, I18nMessageTraits);

} // namespace antwika::i18n::conformance
