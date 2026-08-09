#pragma once

#include <stdexcept>

namespace antwika::music_editor
{

    class ScoreError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
