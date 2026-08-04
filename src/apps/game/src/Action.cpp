#include "antwika/game/Action.hpp"

#include <antwika/replay/NameTable.hpp>

#include "antwika/game/MessageId.hpp"

namespace antwika::game
{

    namespace
    {
        // Tables rather than switches, for footprintOf()'s reason.
        // Every entry is addressed by actionIndex().
        // So an action added to the enumeration is a compile error.
        // Rather than an arm somebody forgot.
        constexpr antwika::replay::NameTable<Action, kActionCount>
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
    } // namespace

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

} // namespace antwika::game
