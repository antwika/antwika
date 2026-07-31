#pragma once

#include <stdexcept>

namespace antwika::ttf
{

    /**
     * @brief Thrown when bytes are not a font this library can read, or
     * when a caller asks for something a font cannot answer.
     *
     * One catchable type per failure category, exactly as
     * antwika::gfx::GfxError and antwika::sound::SoundError are.
     * A caller never learns from it which rasteriser it was built
     * against.
     */
    class TtfError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::ttf
