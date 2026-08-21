#pragma once

#include "antwika/gfx/IShader.hpp"

namespace antwika::gfx::detail
{

    class NullShader final : public IShader
    {
    public:
        NullShader() = default;

        NullShader(const NullShader &) = delete;
        NullShader(NullShader &&) = delete;

        NullShader &operator=(const NullShader &) = delete;
        NullShader &operator=(NullShader &&) = delete;

        [[nodiscard]] bool isReady() const override;
    };

}
