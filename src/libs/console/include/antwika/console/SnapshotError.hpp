#pragma once

#include <stdexcept>

namespace antwika::console
{

    class SnapshotError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
