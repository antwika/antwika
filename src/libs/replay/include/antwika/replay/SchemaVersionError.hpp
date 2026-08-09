#pragma once

#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay
{

    class SchemaVersionError final : public ReplayFormatError
    {
    public:
        using ReplayFormatError::ReplayFormatError;
    };

}
