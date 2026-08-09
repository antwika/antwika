#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/ui_demo/MessageId.hpp"
#include "antwika/ui_demo/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::ui_demo::MessageId;

        constexpr std::array<MessageId, 1> kSameInBoth{
            MessageId::PageLayout,
        };

        struct UiDemoMessageTraits final
        {
            using Messages = antwika::ui_demo::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        UiDemo, MessageSetCompletenessTest, UiDemoMessageTraits);

}
