#pragma once

namespace antwika::gfx
{

    class IRenderer;

    class ClipScope final
    {
    public:
        ~ClipScope();

        ClipScope(const ClipScope &) = delete;
        ClipScope(ClipScope &&) = delete;

        ClipScope &operator=(const ClipScope &) = delete;
        ClipScope &operator=(ClipScope &&) = delete;

    private:
        friend class IRenderer;

        explicit ClipScope(IRenderer &renderer) noexcept;

        IRenderer &renderer;
    };

}
