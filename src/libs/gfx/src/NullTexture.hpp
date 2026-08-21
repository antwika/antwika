#pragma once

#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx::detail
{

    class NullTexture final : public ITexture
    {
    public:
        explicit NullTexture(Size size);

        NullTexture(const NullTexture &) = delete;
        NullTexture(NullTexture &&) = delete;

        NullTexture &operator=(const NullTexture &) = delete;
        NullTexture &operator=(NullTexture &&) = delete;

        [[nodiscard]] Size size() const override;

    private:
        Size textureSize;
    };

}
