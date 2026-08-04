#pragma once

#include <stdexcept>

namespace antwika::music_editor
{

    /**
     * @brief A state dump this application cannot encode, decode or
     * apply.
     *
     * Its own header rather than a corner of StateDump.hpp, because
     * Playback::restore() refuses a broken memory with it and the
     * codec headers include Playback.hpp -- the error is the one piece
     * both sides name.
     */
    class StateDumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::music_editor
