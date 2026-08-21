#pragma once

#include "antwika/gfx/IRenderTarget.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx::fakes
{

    class FakeBareTarget final : public IRenderTarget
    {
    public:
        [[nodiscard]] Size size() const override
        {
            return Size{};
        }

        [[nodiscard]] const ITexture *color() const override
        {
            return nullptr;
        }

        [[nodiscard]] const ITexture *depth() const override
        {
            return nullptr;
        }
    };

}
