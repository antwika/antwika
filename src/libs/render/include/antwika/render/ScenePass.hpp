#pragma once

#include <functional>
#include <memory>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>

namespace antwika::render
{

    inline constexpr float kBloomStrength = 0.85F;

    class ScenePass final
    {
    public:
        void open(
            gfx::ViewportRenderer &viewportRenderer,
            const gfx::ShaderSource &bloomSource);

        void draw(
            gfx::ViewportRenderer &viewportRenderer,
            gfx::IShader &voxelShader,
            gfx::Color backgroundColor,
            const std::function<void()> &pile,
            const std::function<void()> &afterPass);

    private:
        std::unique_ptr<gfx::IRenderTarget> sceneTarget;
        std::unique_ptr<gfx::IRenderTarget> glowTarget;
        std::unique_ptr<gfx::IShader> bloomShader;
        std::unique_ptr<gfx::IMesh> screenMesh;
    };

}
