#pragma once

#include <raylib.h>

#include <antwika/gfx/IShader.hpp>

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibShader final : public IShader
    {
    public:
        RaylibShader(RaylibRenderer &ownerRenderer, ::Shader shader);

        RaylibShader(const RaylibShader &) = delete;
        RaylibShader(RaylibShader &&) = delete;

        RaylibShader &operator=(const RaylibShader &) = delete;
        RaylibShader &operator=(RaylibShader &&) = delete;

        ~RaylibShader() override;

        [[nodiscard]] bool isReady() const override;

        [[nodiscard]] bool isOwnedBy(
            const RaylibRenderer &candidateRenderer) const noexcept;

        [[nodiscard]] const ::Shader &getRawHandle() const noexcept;

        [[nodiscard]] bool isLoaded() const noexcept;

        void untrackRenderer() noexcept;

    private:
        RaylibRenderer *owner;
        ::Shader shader;
        bool loaded = true;
    };

}
