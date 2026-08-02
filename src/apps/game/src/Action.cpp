#include "antwika/game/Action.hpp"

namespace antwika::game
{

    using antwika::i18n::MessageId;

    namespace
    {
        // Tables rather than switches, for footprintOf()'s reason.
        // Every entry is addressed by actionIndex().
        // So an action added to the enumeration is a compile error.
        // Rather than an arm somebody forgot.
        constexpr std::array<std::string_view, kActionCount> kNames{
            "pause", "zoom_in", "zoom_out", "reset_view"};

        constexpr std::array<MessageId, kActionCount> kLabels{
            MessageId::GameActionPause,
            MessageId::GameActionZoomIn,
            MessageId::GameActionZoomOut,
            MessageId::GameActionResetView};
    } // namespace

    std::string_view actionName(Action action) noexcept
    {
        return kNames[actionIndex(action)];
    }

    std::optional<Action> actionFromName(std::string_view name) noexcept
    {
        for (const auto action : kActions)
        {
            if (actionName(action) == name)
            {
                return action;
            }
        }

        return std::nullopt;
    }

    MessageId actionLabel(Action action) noexcept
    {
        return kLabels[actionIndex(action)];
    }

} // namespace antwika::game
