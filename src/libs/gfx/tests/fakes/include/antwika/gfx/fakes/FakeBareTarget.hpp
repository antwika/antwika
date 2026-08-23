#pragma once

#include "antwika/gfx/IRenderTarget.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx::fakes
{

    class FakeBareTarget final : public IRenderTarget
    {
    public:
        [[nodiscard]] Size getSize() const override
        {
            return Size{};
        }

        [[nodiscard]] const ITexture *getColor() const override
        {
            return nullptr;
        }

        [[nodiscard]] const ITexture *getDepth() const override
        {
            return nullptr;
        }
    };

}
