#pragma once

namespace antwika::gfx
{

    class IShader
    {
    public:
        virtual ~IShader() = default;

        [[nodiscard]] virtual bool isReady() const = 0;
    };

}
