#pragma once

#include "antwika/i18n/CompiledMessages.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/MessageTable.hpp"

namespace antwika::i18n
{

    /**
     * @brief This library's own names and both locales' text.
     *
     * Defined in Messages.cpp, where the static_assert that nothing in
     * it is missing reads it.
     */
    extern const MessageTable<MessageId> kMessageTable;

    /**
     * @brief This library's own ids, as a MessageSet.
     *
     * The shape every module repeats: an id type, the list of every id
     * there is, and one catalogue per locale.
     * Here it carries only the language names, which is the whole of
     * what this library has to say in words of its own.
     */
    using Messages = CompiledMessages<MessageId, kMessageTable>;

} // namespace antwika::i18n
