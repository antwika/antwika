#pragma once

#include <optional>
#include <string>

#include "antwika/ui_demo/MessageId.hpp"

namespace antwika::ui_demo
{

    struct DemoMessage final
    {
        MessageId id{MessageId::Cancelled};

        std::string datum;

        std::optional<MessageId> argId;
    };

}
