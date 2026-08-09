#pragma once

#include <cstdint>

namespace antwika::atlas_editor
{

    enum class Modal : std::uint8_t
    {
        None = 0,

        Save,

        Load,

        New,
    };

}
