#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <vector>

#include <antwika/app/FramePreview.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>

using antwika::app::readPngFile;
using antwika::app::writtenPreview;
using antwika::gfx::Bitmap;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::Size;

namespace
{
    [[nodiscard]] Bitmap page()
    {
        constexpr Size kSize{.width = 3, .height = 2};

        return Bitmap{
            .size = kSize,
            .pixels = std::vector<std::uint8_t>(
                kSize.width * kSize.height * kBytesPerPixel, 200)};
    }
}

TEST(FramePreviewTest, WrittenPreview_PutsTheFrameBesideTheExecutable)
{
    const auto path = writtenPreview(page(), "frame-preview-test");

    EXPECT_EQ(
        std::filesystem::path(path).parent_path().filename(), "preview");
    EXPECT_EQ(
        std::filesystem::path(path).filename(),
        "frame-preview-test.png");
}

TEST(FramePreviewTest, WrittenPreview_WritesAPngThatReadsBackTheSame)
{
    const auto path = writtenPreview(page(), "frame-preview-read-back");

    EXPECT_EQ(readPngFile(path, "test"), page());
}
