#pragma once

#include <string>

#include <antwika/input/Key.hpp>

namespace antwika::input
{

    [[nodiscard]] std::string charTypedBy(
        Key key, bool shiftHeld);

}
