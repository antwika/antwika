#pragma once

#include <string>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    struct StatusMessage final
    {
        MessageId id{MessageId::Loaded};

        std::string detail;
    };

}
