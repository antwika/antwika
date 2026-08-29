#pragma once

#include <string>

namespace antwika::editor
{

    struct FileChoice final
    {
        bool isSaveMode = false;

        std::string path;
    };

}
