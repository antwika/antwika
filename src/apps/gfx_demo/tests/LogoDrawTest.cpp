#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/gfx_demo/DemoScene.hpp"

namespace
{
    using antwika::gfx::Bitmap;
    using antwika::gfx::kBytesPerPixel;
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::gfx::mocks::MockTexture;
    using antwika::gfx_demo::DemoScene;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr Size kCanvas{.width = 800, .height = 600};

    [[nodiscard]] Bitmap chequeredLogo()
    {
        constexpr std::uint32_t kSide = 16;

        Bitmap logo;
        logo.size = Size{.width = kSide, .height = kSide};
        logo.pixels.assign(kSide * kSide * kBytesPerPixel, 0);

        for (std::uint32_t y = 0; y < kSide; ++y)
        {
            for (std::uint32_t x = 0; x < kSide; ++x)
            {
                const auto at = (y * kSide + x) * kBytesPerPixel;
                const bool light = ((x / 4) + (y / 4)) % 2 == 0;

                logo.pixels[at] = light ? 240 : 90;
                logo.pixels[at + 1] = light ? 190 : 60;
                logo.pixels[at + 2] = light ? 80 : 140;
                logo.pixels[at + 3] = 255;
            }
        }

        return logo;
    }
}

TEST(LogoDrawTest, Draw_DrawsTheDemoWithItsLogo)
{
    NiceMock<MockRenderer> renderer;

    ON_CALL(renderer, createTexture(_))
        .WillByDefault(
            [](const Bitmap &)
            { return std::make_unique<NiceMock<MockTexture>>(); });

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const auto logo = renderer.createTexture(chequeredLogo());

    const DemoScene scene;
    const auto frame = scene.describe(kCanvas);

    scene.draw(renderer, kCanvas, *logo, frame.commands);
}
