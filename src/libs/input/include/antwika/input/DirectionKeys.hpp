#pragma once

#include <antwika/component/DirectionKeys.hpp>

#include <antwika/input/Key.hpp>

namespace antwika::input
{

    using antwika::component::DirectionKeys;

    void applyArrowKey(
        DirectionKeys &keys, Key key, bool down) noexcept;

    void applyWasdKey(
        DirectionKeys &keys, Key key, bool down) noexcept;

}
