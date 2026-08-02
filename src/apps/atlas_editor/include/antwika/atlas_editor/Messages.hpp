#pragma once

#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    /**
     * @brief This module's ids and catalogues, as an i18n::MessageSet.
     *
     * The shape every module that shows text repeats: an id type, the
     * list of every id there is, and one catalogue per locale.
     * MessagesTest.cpp instantiates the shared completeness suite over
     * it, which is what makes a locale missing an entry a red build.
     */
    struct Messages final
    {
        /**
         * @brief The ids this set answers for.
         */
        using Id = MessageId;

        /**
         * @brief Every id there is, with the name it was declared under.
         * @return The name table, which outlives every caller.
         */
        [[nodiscard]] static std::span<const i18n::MessageName<MessageId>>
            names() noexcept;

        /**
         * @brief The compiled-in catalogue for a locale.
         * @param locale The locale wanted.
         * @return That locale's catalogue, or the default locale's for a
         *         value that is not one of the enumerators.
         */
        [[nodiscard]] static const i18n::Catalogue<MessageId> &
            catalogueFor(i18n::Locale locale) noexcept;
    };

    /**
     * @brief The translator this module's text is worded through.
     *
     * Injected, never reached for: one is built in `main()` and threaded
     * down as a `const Translator &`, as i18n::Translator explains.
     */
    using Translator = i18n::Translator<Messages>;

} // namespace antwika::atlas_editor
