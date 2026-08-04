#pragma once

#include <antwika/i18n/CompiledMessages.hpp>
#include <antwika/i18n/MessageTable.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    /**
     * @brief This module's names and both locales' text, compiled in.
     *
     * Defined in Messages.cpp, where the static_assert that nothing in
     * it is missing reads it.
     */
    extern const i18n::MessageTable<MessageId> kMessageTable;

    /**
     * @brief This module's ids and catalogues, as an i18n::MessageSet.
     *
     * The shape every module that shows text repeats: an id type, the
     * list of every id there is, and one catalogue per locale.
     * MessagesTest.cpp instantiates the shared completeness suite over
     * it, which is what makes a locale missing an entry a red build.
     */
    using Messages = i18n::CompiledMessages<MessageId, kMessageTable>;

    /**
     * @brief The translator this module's text is worded through.
     *
     * Injected, never reached for: one is built in `main()` and threaded
     * down as a `const Translator &`, as i18n::Translator explains.
     */
    using Translator = i18n::Translator<Messages>;

} // namespace antwika::atlas_editor
