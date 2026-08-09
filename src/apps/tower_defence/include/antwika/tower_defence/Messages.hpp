#pragma once

#include <antwika/i18n/CompiledMessages.hpp>
#include <antwika/i18n/MessageTable.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/tower_defence/MessageId.hpp"

namespace antwika::tower_defence
{

    extern const i18n::MessageTable<MessageId> kMessageTable;

    using Messages = i18n::CompiledMessages<MessageId, kMessageTable>;

    using Translator = i18n::Translator<Messages>;

}
