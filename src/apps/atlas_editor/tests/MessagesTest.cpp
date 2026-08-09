#include <array>
#include <span>

#include <antwika/i18n/conformance/MessageSetCompletenessTest.hpp>

#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"

namespace antwika::i18n::conformance
{

    namespace
    {

        using antwika::atlas_editor::MessageId;

        constexpr std::array<MessageId, 2> kSameInBoth{
            MessageId::PixelUnknown,
            MessageId::PixelAt,
        };

        struct AtlasEditorMessageTraits final
        {
            using Messages = antwika::atlas_editor::Messages;

            static std::span<const MessageId> sameInBothLocales()
            {
                return kSameInBoth;
            }
        };
    }

    INSTANTIATE_TYPED_TEST_SUITE_P(
        AtlasEditor, MessageSetCompletenessTest, AtlasEditorMessageTraits);

}
