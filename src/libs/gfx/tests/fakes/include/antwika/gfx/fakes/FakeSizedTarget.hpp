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

        [[nodiscard]] Size getSize() const override
        {
            return askedSize;
        }

        [[nodiscard]] const ITexture *getColor() const override
        {
            return nullptr;
        }

        [[nodiscard]] const ITexture *getDepth() const override
        {
            return nullptr;
        }

    private:
        Size askedSize;
    };

}
