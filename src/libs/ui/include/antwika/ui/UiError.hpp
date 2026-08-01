#pragma once

#include <stdexcept>

namespace antwika::ui
{

    /**
     * @brief Thrown when a frame is asked for something it cannot give,
     * such as the picture of a layout that is still half-built.
     *
     * One catchable type per failure category, exactly as
     * antwika::gfx::GfxError and antwika::font::FontError are.
     *
     * This library refuses almost nothing, and deliberately so: a
     * mis-nested layout is not expressible rather than checked, since a
     * Scope is the only way to close a container and it closes the one
     * it was opened for. So there is one thing left for this type to
     * report, and it is the one thing a Scope cannot see.
     * See Context::finish().
     */
    class UiError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::ui
