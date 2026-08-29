#pragma once

#include <string>

namespace antwika::editor
{

    struct FileDialog final
    {
        bool isSaveMode = false;

        std::string folder;

        std::string fileName;
    };

}
