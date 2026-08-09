#include "antwika/tower_defence/Messages.hpp"

#include <antwika/i18n/MessageTable.hpp>

#include "antwika/tower_defence/MessageId.hpp"

namespace antwika::tower_defence
{

    constexpr i18n::MessageTable<MessageId> kMessageTable{
        .names{{
            {MessageId::Level, "Level"},
            {MessageId::Wave, "Wave"},
            {MessageId::Lives, "Lives"},
            {MessageId::Score, "Score"},
            {MessageId::Best, "Best"},
            {MessageId::Cleared, "Cleared"},
            {MessageId::Overrun, "Overrun"},
        }},
        .english{{
            {MessageId::Level, "lvl {0}/{1}"},
            {MessageId::Wave, "wave {0}/{1}"},
            {MessageId::Lives, "lives {0}"},
            {MessageId::Score, "score {0}"},
            {MessageId::Best, "best {0}"},
            {MessageId::Cleared, "CLEARED"},
            {MessageId::Overrun, "OVERRUN"},
        }},
        .swedish{{
            {MessageId::Level, "nivå {0}/{1}"},
            {MessageId::Wave, "våg {0}/{1}"},
            {MessageId::Lives, "liv {0}"},
            {MessageId::Score, "poäng {0}"},
            {MessageId::Best, "bäst {0}"},
            {MessageId::Cleared, "KLARAT"},
            {MessageId::Overrun, "ÖVERMANNAD"},
        }},
    };

    static_assert(
        i18n::isComplete(kMessageTable),
        "every MessageId needs a name and text in both locales");

}
