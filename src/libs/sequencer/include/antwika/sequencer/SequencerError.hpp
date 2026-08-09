#pragma once

#include <stdexcept>

namespace antwika::sequencer
{

    class SequencerError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
