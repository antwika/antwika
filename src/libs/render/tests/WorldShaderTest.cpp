#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/CubeFace.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockShader.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/light/PointLight.hpp>

#include "antwika/render/WorldShader.hpp"
#include "antwika/render/WorldShaderInputs.hpp"

using antwika::render::WorldShader;
using antwika::render::WorldShaderInputs;
using antwika::gfx::IShader;
using antwika::gfx::ShaderSource;
using antwika::gfx::Vec3;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockShader;
using antwika::light::ActiveLight;
using ::testing::NiceMock;

namespace
{
    constexpr std::array<std::string_view, 40> kLampUniformNames{
        "lampAt[0]",
        "lampAt[1]",
        "lampAt[2]",
        "lampAt[3]",
        "lampAt[4]",
        "lampAt[5]",
        "lampAt[6]",
        "lampAt[7]",
        "lampBrightness[0]",
        "lampBrightness[1]",
        "lampBrightness[2]",
        "lampBrightness[3]",
        "lampBrightness[4]",
        "lampBrightness[5]",
        "lampBrightness[6]",
        "lampBrightness[7]",
        "lampReach[0]",
        "lampReach[1]",
        "lampReach[2]",
        "lampReach[3]",
        "lampReach[4]",
        "lampReach[5]",
        "lampReach[6]",
        "lampReach[7]",
        "lampShadows[0]",
        "lampShadows[1]",
        "lampShadows[2]",
        "lampShadows[3]",
        "lampShadows[4]",
        "lampShadows[5]",
        "lampShadows[6]",
        "lampShadows[7]",
        "lampTint[0]",
        "lampTint[1]",
        "lampTint[2]",
        "lampTint[3]",
        "lampTint[4]",
        "lampTint[5]",
        "lampTint[6]",
        "lampTint[7]"};

    void handsOutShaders(NiceMock<MockRenderer> &innerRenderer)
    {
        ON_CALL(innerRenderer, createShader(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::ShaderSource
                       &source)
                {
                    return std::unique_ptr<IShader>{
                        std::make_unique<NiceMock<MockShader>>()};
                });
    }

    void gathersUniformNames(
        NiceMock<MockRenderer> &innerRenderer,
        std::set<std::string> &seenNames)
    {
        ON_CALL(
            innerRenderer,
            setShaderNumber(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                [&seenNames](
                    [[maybe_unused]] const IShader &target,
                    const std::string_view name,
                    [[maybe_unused]] const float value)
                { seenNames.insert(std::string{name}); });
        ON_CALL(
            innerRenderer,
            setShaderVector(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                [&seenNames](
                    [[maybe_unused]] const IShader &target,
                    const std::string_view name,
                    [[maybe_unused]] const Vec3 value)
                { seenNames.insert(std::string{name}); });
        ON_CALL(
            innerRenderer,
            setShaderColor(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                [&seenNames](
                    [[maybe_unused]] const IShader &target,
                    const std::string_view name,
                    [[maybe_unused]] const antwika::gfx::Color value)
                { seenNames.insert(std::string{name}); });
    }
}

TEST(WorldShaderTest, Open_TakesUpTheVoxelShaderItHandsOut)
{
    NiceMock<MockRenderer> innerRenderer;
    IShader *voxelShader = nullptr;

    ON_CALL(innerRenderer, createShader(::testing::_))
        .WillByDefault(
            [&voxelShader](
                [[maybe_unused]] const antwika::gfx::ShaderSource
                    &source)
            {
                auto shader =
                    std::make_unique<NiceMock<MockShader>>();

                voxelShader = shader.get();

                return std::unique_ptr<IShader>{std::move(shader)};
            });

    WorldShader worldShader;

    worldShader.open(innerRenderer, ShaderSource{});

    EXPECT_EQ(&worldShader.getProgram(), voxelShader);
}

TEST(WorldShaderTest, Open_TellsTheShaderTheShadowAtlasShape)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::map<std::string, float> seenNumbers;

    ON_CALL(
        innerRenderer,
        setShaderNumber(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(
            [&seenNumbers](
                [[maybe_unused]] const IShader &target,
                const std::string_view name,
                const float value)
            { seenNumbers[std::string{name}] = value; });

    WorldShader worldShader;

    worldShader.open(innerRenderer, ShaderSource{});

    EXPECT_EQ(
        seenNumbers.at("lampSlots"),
        static_cast<float>(antwika::light::kMaxLamps));
    EXPECT_EQ(
        seenNumbers.at("lampFaces"),
        static_cast<float>(antwika::gfx::kCubeFaces));
}

TEST(WorldShaderTest, SetLook_NamesEveryLampUniformSlotByItsIndex)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::set<std::string> seenNames;
    gathersUniformNames(innerRenderer, seenNames);

    WorldShader worldShader;
    const std::vector<ActiveLight> lights(8, ActiveLight{});

    worldShader.open(innerRenderer, ShaderSource{});
    worldShader.setLook(innerRenderer, WorldShaderInputs{}, lights, {});

    for (const auto name : kLampUniformNames)
    {
        EXPECT_TRUE(seenNames.contains(std::string{name})) << name;
    }
}

TEST(WorldShaderTest, SetLook_LeavesTheSlotsPastTheLitLampsUnnamed)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::set<std::string> seenNames;
    gathersUniformNames(innerRenderer, seenNames);

    WorldShader worldShader;
    const std::vector<ActiveLight> lights(1, ActiveLight{});

    worldShader.open(innerRenderer, ShaderSource{});
    worldShader.setLook(innerRenderer, WorldShaderInputs{}, lights, {});

    EXPECT_TRUE(seenNames.contains("lampAt[0]"));
    EXPECT_FALSE(seenNames.contains("lampAt[1]"));
    EXPECT_FALSE(seenNames.contains("lampShadows[1]"));
}

namespace
{
    void gathersUniformVectors(
        NiceMock<MockRenderer> &innerRenderer,
        std::map<std::string, Vec3> &seenVectors)
    {
        ON_CALL(
            innerRenderer,
            setShaderVector(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                [&seenVectors](
                    [[maybe_unused]] const IShader &target,
                    const std::string_view name,
                    const Vec3 value)
                { seenVectors[std::string{name}] = value; });
    }

    void gathersUniformNumbers(
        NiceMock<MockRenderer> &innerRenderer,
        std::map<std::string, float> &seenNumbers)
    {
        ON_CALL(
            innerRenderer,
            setShaderNumber(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(
                [&seenNumbers](
                    [[maybe_unused]] const IShader &target,
                    const std::string_view name,
                    const float value)
                { seenNumbers[std::string{name}] = value; });
    }
}

TEST(WorldShaderTest, Open_TellsTheShaderHowFarTheFogReaches)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::map<std::string, float> seenNumbers;
    gathersUniformNumbers(innerRenderer, seenNumbers);

    WorldShader worldShader;

    worldShader.open(innerRenderer, ShaderSource{});

    EXPECT_EQ(seenNumbers.at("fogNear"), antwika::render::kFogNear);
    EXPECT_EQ(seenNumbers.at("fogFar"), antwika::render::kFogFar);
}

TEST(WorldShaderTest, SetLook_MeasuresTheFogFromWhatTheCameraAimsAt)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::map<std::string, Vec3> seenVectors;
    gathersUniformVectors(innerRenderer, seenVectors);

    WorldShader worldShader;

    worldShader.open(innerRenderer, ShaderSource{});
    worldShader.setLook(
        innerRenderer,
        WorldShaderInputs{
            .playing = true,
            .viewPosition = Vec3{0.0F, 0.0F, 20.0F},
            .viewTargetPoint = Vec3{0.0F, 0.0F, 4.0F}},
        {},
        {});

    EXPECT_EQ(seenVectors.at("fogFrom"), (Vec3{0.0F, 0.0F, 4.0F}));
    EXPECT_EQ(seenVectors.at("fogWay"), (Vec3{0.0F, 0.0F, -1.0F}));
}

TEST(WorldShaderTest, SetLook_AimsTheFogAheadWhereTheCameraStandsOnIt)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::map<std::string, Vec3> seenVectors;
    gathersUniformVectors(innerRenderer, seenVectors);

    WorldShader worldShader;

    worldShader.open(innerRenderer, ShaderSource{});
    worldShader.setLook(innerRenderer, WorldShaderInputs{}, {}, {});

    EXPECT_EQ(seenVectors.at("fogWay"), (Vec3{0.0F, 0.0F, -1.0F}));
}

TEST(WorldShaderTest, SetLook_LaysTheFogOnWhilePlaying)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::map<std::string, float> seenNumbers;
    gathersUniformNumbers(innerRenderer, seenNumbers);

    WorldShader worldShader;

    worldShader.open(innerRenderer, ShaderSource{});
    worldShader.setLook(
        innerRenderer, WorldShaderInputs{.playing = true}, {}, {});

    EXPECT_EQ(seenNumbers.at("fogStrength"), antwika::render::kFogStrength);
}

TEST(WorldShaderTest, SetLook_LeavesTheFogOutWhileEditing)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::map<std::string, float> seenNumbers;
    gathersUniformNumbers(innerRenderer, seenNumbers);

    WorldShader worldShader;

    worldShader.open(innerRenderer, ShaderSource{});
    worldShader.setLook(
        innerRenderer, WorldShaderInputs{.playing = false}, {}, {});

    EXPECT_EQ(seenNumbers.at("fogStrength"), 0.0F);
}

TEST(WorldShaderTest, SetLook_TintsTheFogWithTheBackdropItDrawsOn)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutShaders(innerRenderer);
    std::map<std::string, antwika::gfx::Color> seenColors;

    ON_CALL(
        innerRenderer,
        setShaderColor(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(
            [&seenColors](
                [[maybe_unused]] const IShader &target,
                const std::string_view name,
                const antwika::gfx::Color value)
            { seenColors[std::string{name}] = value; });

    WorldShader worldShader;
    constexpr antwika::gfx::Color backdropColor{
        .red = 6, .green = 6, .blue = 10, .alpha = 255};

    worldShader.open(innerRenderer, ShaderSource{});
    worldShader.setLook(
        innerRenderer,
        WorldShaderInputs{.playing = true, .backdropColor = backdropColor},
        {},
        {});

    EXPECT_EQ(seenColors.at("fogTint"), backdropColor);
}
