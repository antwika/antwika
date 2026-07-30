#include "antwika/i18n/MessageId.hpp"

#include <string_view>

namespace antwika::i18n
{

    std::string_view nameOf(MessageId id) noexcept
    {
        switch (id)
        {
        case MessageId::MenuPlayGame:
            return "MenuPlayGame";
        case MessageId::MenuLoadReplay:
            return "MenuLoadReplay";
        case MessageId::MenuSaveReplay:
            return "MenuSaveReplay";
        case MessageId::MenuResumeGame:
            return "MenuResumeGame";
        case MessageId::MenuLanguage:
            return "MenuLanguage";
        case MessageId::LanguageEnglish:
            return "LanguageEnglish";
        case MessageId::LanguageSwedish:
            return "LanguageSwedish";
        case MessageId::ToolbarZoomIn:
            return "ToolbarZoomIn";
        case MessageId::ToolbarZoomOut:
            return "ToolbarZoomOut";
        case MessageId::ToolbarResetView:
            return "ToolbarResetView";
        case MessageId::ToolbarZoomLevel:
            return "ToolbarZoomLevel";
        }

        return "?";
    }

} // namespace antwika::i18n
