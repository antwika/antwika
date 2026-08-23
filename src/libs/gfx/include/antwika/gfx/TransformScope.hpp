#pragma once

namespace antwika::gfx
{

    class IRenderer;

    class TransformScope final
    {
    public:
        ~TransformScope();

        TransformScope(const TransformScope &) = delete;
        TransformScope(TransformScope &&) = delete;

        TransformScope &operator=(const TransformScope &) = delete;
        TransformScope &operator=(TransformScope &&) = delete;

    private:
        friend class IRenderer;

        explicit TransformScope(IRenderer &renderer) noexcept;

        IRenderer &renderer;
    };

}
