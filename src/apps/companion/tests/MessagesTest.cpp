#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompleteness.hpp>

#include "antwika/companion/MessageId.hpp"
#include "antwika/companion/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::companion::MessageId;

        // The ids whose two texts genuinely read the same in both.
        // A notation, a loanword or a noise an animal makes.
        // Written out rather than tolerated wherever it happens.
        // A forgotten Swedish entry looks exactly like one of these.
        // So the only way to be excused is to be named here.
        constexpr std::array<MessageId, 5> kSameInBoth{
            MessageId::Hunger,
            MessageId::SayLaLaLa,
            MessageId::SayZzz,
            MessageId::SayWheee,
            MessageId::Day,
        };

        /**
         * @brief This module's ids, for the shared completeness suite.
         */
        struct CompanionMessageTraits
        {
            using Messages = antwika::companion::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Companion, MessageSetCompleteness, CompanionMessageTraits);

} // namespace antwika::i18n::conformance
