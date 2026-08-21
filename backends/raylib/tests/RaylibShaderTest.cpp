#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "RaylibBackend.hpp"

using antwika::gfx::GfxError;
using antwika::gfx::IShader;
using antwika::gfx::ShaderSource;
using antwika::gfx::WindowSpec;
using antwika::gfx::raylib::RaylibBackend;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    WindowSpec demoSpec()
    {
        return WindowSpec{
            .title = "Antwika raylib shaders",
            .size = {.width = 320, .height = 240},
            .hidden = true};
    }

    ShaderSource workingSource()
    {
        return ShaderSource{
            .vertex = R"(#version 330

in vec3 vertexPosition;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)",
            .fragment = R"(#version 330

out vec4 finalColor;

void main()
{
    finalColor = vec4(1.0);
}
)"};
    }
}

TEST(RaylibShaderTest, CreateShader_ThrowsWhenTheVertexStageWillNotCompile)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(demoSpec());
    auto &renderer = window->renderer();

    ShaderSource brokenSource = workingSource();
    brokenSource.vertex = "#version 330\nthis is not glsl\n";

    EXPECT_THROW(
        { const auto shader = renderer.createShader(brokenSource); }, GfxError);
}

TEST(RaylibShaderTest, CreateShader_ThrowsWhenTheFragmentStageWillNotCompile)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(demoSpec());
    auto &renderer = window->renderer();

    ShaderSource brokenSource = workingSource();
    brokenSource.fragment = "#version 330\nvoid main() { nonsense(); }\n";

    EXPECT_THROW(
        { const auto shader = renderer.createShader(brokenSource); }, GfxError);
}

TEST(RaylibShaderTest, CreateShader_ThrowsOnceTheWindowHasClosed)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(demoSpec());
    auto &renderer = window->renderer();

    window->close();

    EXPECT_THROW(
        { const auto shader = renderer.createShader(workingSource()); },
        GfxError);
}

TEST(RaylibShaderTest, Shader_StopsBeingReadyOnceItsWindowCloses)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(demoSpec());
    const std::unique_ptr<IShader> shader =
        window->renderer().createShader(workingSource());

    ASSERT_NE(nullptr, shader);
    EXPECT_TRUE(shader->isReady());

    window->close();

    EXPECT_FALSE(shader->isReady());
}
