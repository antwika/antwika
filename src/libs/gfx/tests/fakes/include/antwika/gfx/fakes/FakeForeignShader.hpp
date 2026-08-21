#pragma once

#include "antwika/gfx/IShader.hpp"

namespace antwika::gfx::fakes
{

    class FakeForeignShader final : public IShader
    {
    public:
        [[nodiscard]] bool isReady() const override
        {
            return true;
        }
    };

}
