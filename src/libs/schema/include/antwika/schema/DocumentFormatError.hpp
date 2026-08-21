#pragma once

#include <stdexcept>

namespace antwika::schema
{

    class DocumentFormatError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
