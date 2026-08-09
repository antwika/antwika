#pragma once
#include <antwika/event/ITickEventSource.hpp>

namespace antwika::game::events
{

    using antwika::event::ITickEventSource;

    inline constexpr const char *kScoreIncrement = "game.score_increment";

    inline constexpr const char *kBindKey = "game.bind_key";

    inline constexpr const char *kSetLocale = "game.set_locale";

    inline constexpr const char *kSetKeyboard = "game.set_keyboard";

}
