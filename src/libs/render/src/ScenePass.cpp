#include "antwika/render/ScenePass.hpp"

#include <cstdint>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/render/DoubleSidedTriangles.hpp"

namespace antwika::render
{

    namespace
    {
    [[nodiscard]] gfx::MeshData getScreenQuad()
    {
        gfx::MeshData mesh;

        mesh.vertices.push_back(
            gfx::Vertex3D{
                .position = {-1.0F, -1.0F, 0.0F},
                .texCoordinate = {0.0F, 0.0F}});
        mesh.vertices.push_back(
            gfx::Vertex3D{
                .position = {1.0F, -1.0F, 0.0F},
                .texCoordinate = {1.0F, 0.0F}});
        mesh.vertices.push_back(
            gfx::Vertex3D{
                .position = {1.0F, 1.0F, 0.0F},
                .texCoordinate = {1.0F, 1.0F}});
        mesh.vertices.push_back(
            gfx::Vertex3D{
                .position = {-1.0F, 1.0F, 0.0F},
                .texCoordinate = {0.0F, 1.0F}});

        layDoubleSidedTriangle(mesh, {1U, 2U, 0U});
        layDoubleSidedTriangle(mesh, {2U, 3U, 0U});

        return mesh;
    } // GCOVR_EXCL_LINE
    }

    void ScenePass::open(
        gfx::ViewportRenderer &viewportRenderer,
        const gfx::ShaderSource &bloomSource)
    {
        bloomShader = viewportRenderer.createShader(bloomSource);
        screenMesh = viewportRenderer.createMesh(getScreenQuad());
    }

    void ScenePass::draw(
        gfx::ViewportRenderer &viewportRenderer,
        gfx::IShader &voxelShader,
        const gfx::Color backgroundColor,
        const std::function<void()> &pile,
        const std::function<void()> &afterPass)
    {
        const auto port = viewportRenderer.getViewport();
        const gfx::Size framePixelsSize{
            .width = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(camera::kCanvasSize.width)
                * port.numerator / port.denominator),
            .height = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(camera::kCanvasSize.height)
                * port.numerator / port.denominator)};

        if (!sceneTarget || sceneTarget->getSize() != framePixelsSize)
        {
            sceneTarget = viewportRenderer.createRenderTarget(
                gfx::RenderTargetSpec{
                    .size = framePixelsSize, .depth = true});
        }

        if (!glowTarget || glowTarget->getSize() != framePixelsSize)
        {
            glowTarget = viewportRenderer.createRenderTarget(
                gfx::RenderTargetSpec{
                    .size = framePixelsSize, .depth = true});
        }

        viewportRenderer.setShaderNumber(voxelShader, "glowOnly", 1.0F);

        {
            const auto scope = viewportRenderer.targetScope(*glowTarget);

            viewportRenderer.clear(gfx::Color{});
            pile();
        }

        viewportRenderer.setShaderNumber(voxelShader, "glowOnly", 0.0F);

        {
            const auto scope = viewportRenderer.targetScope(*sceneTarget);

            viewportRenderer.clear(backgroundColor);
            pile();
            afterPass();
        }

        const gfx::Camera3D screenCamera{
            gfx::Vec3{0.0F, 0.0F, 0.0F},
            gfx::Vec3{0.0F, 0.0F, -1.0F},
            gfx::Vec3{0.0F, 1.0F, 0.0F},
            gfx::Orthographic{
                .halfWidth = 1.0F,
                .halfHeight = 1.0F,
                .nearPlane = -1.0F,
                .farPlane = 1.0F}};

        const auto sceneSize = sceneTarget->getSize();

        viewportRenderer.setShaderVector(
            *bloomShader,
            "texelSize",
            gfx::Vec3{
                1.0F / static_cast<float>(sceneSize.width),
                1.0F / static_cast<float>(sceneSize.height),
                0.0F});
        viewportRenderer.setShaderNumber(
            *bloomShader, "bloomStrength", kBloomStrength);
        viewportRenderer.drawMesh(
            *screenMesh,
            gfx::getIdentityMatrix(),
            screenCamera,
            gfx::MeshMaterial{
                .texture = sceneTarget->getColor(),
                .materialMapTexture = glowTarget->getColor(),
                .shader = bloomShader.get()});
    }

}
