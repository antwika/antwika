#pragma once

#include <stdexcept>

namespace antwika::atlas_editor
{

    /**
     * @brief Thrown when this application is asked for something it
     * cannot give, such as a canvas with no pixels in it.
     *
     * One specific, catchable type per failure category, as everywhere
     * else here.
     * A PNG that will not decode or will not be written is
     * antwika::gfx::GfxError's to report and is deliberately not
     * re-wrapped: the graphics library already says exactly what was
     * wrong with the bytes.
     */
    class AtlasEditorError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::atlas_editor
