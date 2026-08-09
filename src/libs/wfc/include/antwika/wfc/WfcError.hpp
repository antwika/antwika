#pragma once

#include <stdexcept>

namespace antwika::wfc
{

    class WfcError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
