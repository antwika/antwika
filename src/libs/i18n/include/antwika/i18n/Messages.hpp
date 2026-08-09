#pragma once

#include "antwika/i18n/CompiledMessages.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/MessageTable.hpp"

namespace antwika::i18n
{

    extern const MessageTable<MessageId> kMessageTable;

    using Messages = CompiledMessages<MessageId, kMessageTable>;

}
