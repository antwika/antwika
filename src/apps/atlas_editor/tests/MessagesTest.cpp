#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompleteness.hpp>

#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::atlas_editor::MessageId;

        // The ids whose two texts genuinely read the same in both.
        // A notation, a loanword or a noise an animal makes.
        // Written out rather than tolerated wherever it happens.
        // A forgotten Swedish entry looks exactly like one of these.
        // So the only way to be excused is to be named here.
        constexpr std::array<MessageId, 2> kSameInBoth{
            MessageId::PixelUnknown,
            MessageId::PixelAt,
        };

        /**
         * @brief This module's ids, for the shared completeness suite.
         */
        struct AtlasEditorMessageTraits
        {
            using Messages = antwika::atlas_editor::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    } // namespace

    INSTANTIATE_TYPED_TEST_SUITE_P(
        AtlasEditor, MessageSetCompleteness, AtlasEditorMessageTraits);

} // namespace antwika::i18n::conformance
