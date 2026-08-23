#pragma once

#include <string>

#include <antwika/input/Key.hpp>

namespace antwika::input
{

    [[nodiscard]] std::string getCharTypedBy(
        Key key, bool shiftHeld);

}
