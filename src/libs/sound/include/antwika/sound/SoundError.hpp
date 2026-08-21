#pragma once

#include <stdexcept>

namespace antwika::sound
{

    class SoundError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
