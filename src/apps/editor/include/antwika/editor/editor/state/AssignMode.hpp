#pragma once

#include <cstddef>

namespace antwika::editor
{

    struct AssignMode final
    {
        bool basePicking = false;

        std::size_t framePicked = 0;

        std::size_t memberPicked = 0;

        bool memberAssigning = false;

        std::size_t flipFramePicked = 0;

        bool flipFrameAssigning = false;

        bool variantPicking = false;
    };

}
