#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/render/CharacterSkins.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::ITexture;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::render::CharacterSkins;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] Bitmap skinOf(const Size size)
    {
        return Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width)
                    * static_cast<std::size_t>(size.height)
                    * antwika::gfx::kBytesPerPixel,
                0)};
    }

    void handsOutSizedTextures(NiceMock<MockRenderer> &renderer)
    {
        ON_CALL(renderer, createTexture)
            .WillByDefault(
                [](const Bitmap &bitmap)
                {
                    auto skinTexture =
                        std::make_unique<NiceMock<MockTexture>>();

                    ON_CALL(*skinTexture, getSize)
                        .WillByDefault(::testing::Return(bitmap.size));

                    return std::unique_ptr<ITexture>{
                        std::move(skinTexture)};
                });
    }
}

TEST(CharacterSkinsTest, Take_MakesATextureForEachSkinItIsGiven)
{
    NiceMock<MockRenderer> renderer;
    handsOutSizedTextures(renderer);
    CharacterSkins skins;

    EXPECT_CALL(renderer, createTexture).Times(2);

    skins.take(
        renderer,
        {skinOf(Size{.width = 8, .height = 8}),
         skinOf(Size{.width = 4, .height = 4})});

    EXPECT_EQ(skins.getSize(), 2U);
    EXPECT_NE(skins.getPicture(0), nullptr);
    EXPECT_NE(skins.getPicture(1), nullptr);
}

TEST(CharacterSkinsTest, Take_UpdatesTheTexturesItHoldsAtTheSameSizes)
{
    NiceMock<MockRenderer> renderer;
    handsOutSizedTextures(renderer);
    CharacterSkins skins;

    skins.take(renderer, {skinOf(Size{.width = 8, .height = 8})});

    EXPECT_CALL(renderer, createTexture).Times(0);
    EXPECT_CALL(renderer, updateTexture).Times(1);

    skins.take(renderer, {skinOf(Size{.width = 8, .height = 8})});
}

TEST(CharacterSkinsTest, Take_DropsTheTexturesPastTheSkinsItIsGiven)
{
    NiceMock<MockRenderer> renderer;
    handsOutSizedTextures(renderer);
    CharacterSkins skins;

    skins.take(
        renderer,
        {skinOf(Size{.width = 8, .height = 8}),
         skinOf(Size{.width = 4, .height = 4})});
    skins.take(renderer, {skinOf(Size{.width = 8, .height = 8})});

    EXPECT_EQ(skins.getSize(), 1U);
    EXPECT_EQ(skins.getPicture(1), nullptr);
}

TEST(CharacterSkinsTest, Lay_UpdatesTheTextureWhenTheSkinKeepsItsSize)
{
    NiceMock<MockRenderer> renderer;
    handsOutSizedTextures(renderer);
    CharacterSkins skins;

    skins.take(renderer, {skinOf(Size{.width = 8, .height = 8})});

    const auto *picture = skins.getPicture(0);

    EXPECT_CALL(renderer, createTexture).Times(0);
    EXPECT_CALL(renderer, updateTexture).Times(1);

    skins.lay(renderer, 0, skinOf(Size{.width = 8, .height = 8}));

    EXPECT_EQ(skins.getPicture(0), picture);
}

TEST(CharacterSkinsTest, Lay_MakesATextureAfreshWhenTheSkinChangesSize)
{
    NiceMock<MockRenderer> renderer;
    handsOutSizedTextures(renderer);
    CharacterSkins skins;

    skins.take(renderer, {skinOf(Size{.width = 8, .height = 8})});

    EXPECT_CALL(renderer, createTexture).Times(1);
    EXPECT_CALL(renderer, updateTexture).Times(0);

    skins.lay(renderer, 0, skinOf(Size{.width = 16, .height = 16}));
}

TEST(CharacterSkinsTest, Lay_LeavesTheSheetsAloneForAnIndexPastThem)
{
    NiceMock<MockRenderer> renderer;
    handsOutSizedTextures(renderer);
    CharacterSkins skins;

    skins.take(renderer, {skinOf(Size{.width = 8, .height = 8})});

    EXPECT_CALL(renderer, createTexture).Times(0);
    EXPECT_CALL(renderer, updateTexture).Times(0);

    skins.lay(renderer, 1, skinOf(Size{.width = 8, .height = 8}));

    EXPECT_EQ(skins.getSize(), 1U);
}
