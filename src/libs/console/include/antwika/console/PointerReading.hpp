#pragma once

#include <antwika/app/PointerReading.hpp>

namespace antwika::console
{

    /**
     * @brief Moved to antwika::app, which is where an application is
     * allowed to say that two libraries knowing nothing of each other
     * describe one thing; the names are kept here on
     * game/PointerReading.hpp's exact terms, so every sink that
     * includes this header goes on compiling.
     *
     * There were two definitions of these two functions, word for word,
     * which is two places to say it differently -- exactly what the
     * comment on each of them said it was there to prevent.
     */
    using antwika::app::asPoint;
    using antwika::app::locates;

} // namespace antwika::console
