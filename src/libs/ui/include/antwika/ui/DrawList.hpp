#pragma once

#include <vector>

#include "antwika/ui/DrawCommand.hpp"

namespace antwika::ui
{

    /**
     * @brief A whole picture, in the order it is drawn.
     *
     * Order is paint order, and it is the order the widgets were
     * declared in: a container's own fill comes before anything inside
     * it, so a background never covers its content.
     * That is all the depth this library has, since antwika::gfx offers
     * no way to draw out of order.
     */
    using DrawList = std::vector<DrawCommand>;

} // namespace antwika::ui
