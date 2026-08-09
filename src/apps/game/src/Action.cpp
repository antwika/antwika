#include "antwika/game/Action.hpp"

#include <antwika/enums/NameTable.hpp>

#include "antwika/game/MessageId.hpp"

namespace antwika::game
{

    namespace
    {
        constexpr antwika::enums::NameTable<Action>
            kNames{
                {"pause",
                 "zoom_in",
                 "zoom_out",
                 "reset_view",
                 "console_toggle",
                 "console_execute"}};

        constexpr std::array<MessageId, kActionCount> kLabels{
            MessageId::ActionPause,
            MessageId::ActionZoomIn,
            MessageId::ActionZoomOut,
            MessageId::ActionResetView,
            MessageId::ActionConsoleToggle,
            MessageId::ActionConsoleExecute};
    }

    std::string_view actionName(Action action) noexcept
    {
        return kNames.name(action);
    }

    std::optional<Action> actionFromName(std::string_view name) noexcept
    {
        return kNames.from(name);
    }

    MessageId actionLabel(Action action) noexcept
    {
        return kLabels[actionIndex(action)];
    }

}
