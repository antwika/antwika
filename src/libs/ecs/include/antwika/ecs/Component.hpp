#pragma once

#include <type_traits>

namespace antwika::ecs
{

    template <typename T>
    concept Component =
        std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

}
