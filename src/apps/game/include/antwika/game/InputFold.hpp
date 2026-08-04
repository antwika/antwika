#pragma once

#include <antwika/console/InputFold.hpp>

namespace antwika::game
{

    /**
     * @brief The fold moved to antwika::console with the debug
     * console; the name is kept here so every sink goes on naming its
     * collaborator as before -- gfx::Size names geometry::Size on the
     * same terms.
     */
    using antwika::console::InputFold;

    // The names the old header declared beside it are kept too.
    // Every sink in this application spells them bare.
    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::InputState;

} // namespace antwika::game
