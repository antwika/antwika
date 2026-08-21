#pragma once

#include <antwika/schema/DocumentFormatError.hpp>

namespace antwika::schema
{

    class SchemaVersionError final : public DocumentFormatError
    {
    public:
        using DocumentFormatError::DocumentFormatError;
    };

}
