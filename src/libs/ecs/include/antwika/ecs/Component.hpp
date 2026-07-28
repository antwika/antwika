#pragma once

#include <type_traits>

namespace antwika::ecs
{

    /**
     * @brief Constrains a type to plain, trivially-copyable data.
     *
     * Components are deliberately just data: no virtual methods, no
     * user-defined copy/move/destructor logic that could hide behavior a
     * system would need to run through instead of a plain memcpy-style
     * copy between the front and back buffers (see ComponentStorage).
     */
    template <typename T>
    concept Component =
        std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

} // namespace antwika::ecs
