#pragma once

#include "antwika/gfx/IRenderTarget.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx::fakes
{

    class FakeSizedTarget final : public IRenderTarget
    {
    public:
        explicit FakeSizedTarget(const Size size) : askedSize(size)
        {
        }

        [[nodiscard]] Size size() const override
        {
            return askedSize;
        }

        [[nodiscard]] const ITexture *color() const override
        {
            return nullptr;
        }

        [[nodiscard]] const ITexture *depth() const override
        {
            return nullptr;
        }

    private:
        Size askedSize;
    };

}
