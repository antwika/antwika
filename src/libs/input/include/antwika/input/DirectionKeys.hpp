#pragma once

#include <antwika/intent/DirectionKeys.hpp>

#include <antwika/input/Key.hpp>

namespace antwika::input
{

    using antwika::intent::DirectionKeys;

    void applyArrowKey(
        DirectionKeys &keys, Key key, bool down) noexcept;

    void applyWasdKey(
        DirectionKeys &keys, Key key, bool down) noexcept;

}
