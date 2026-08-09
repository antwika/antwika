#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/task_worker/MessageId.hpp"
#include "antwika/task_worker/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::task_worker::MessageId;

        constexpr std::array<MessageId, 2> kSameInBoth{
            MessageId::Tick,
            MessageId::Budget,
        };

        struct TaskWorkerMessageTraits final
        {
            using Messages = antwika::task_worker::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TaskWorker, MessageSetCompletenessTest, TaskWorkerMessageTraits);

}
