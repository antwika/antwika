#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/ShaderReader.hpp"
#include "antwika/gfx/ShaderSource.hpp"

using antwika::gfx::GfxError;
using antwika::gfx::ShaderReader;
using antwika::gfx::ShaderSource;

TEST(ShaderReaderTest, ReadStage_KeepsTheTextVerbatim)
{
    std::istringstream inputStream("#version 330\nvoid main() {}\n");

    EXPECT_EQ("#version 330\nvoid main() {}\n",
              ShaderReader{}.readAll(inputStream));
}

TEST(ShaderReaderTest, ReadStage_ThrowsOnAnEmptyStream)
{
    std::istringstream inputStream("");

    EXPECT_THROW({ (void)ShaderReader{}.readAll(inputStream); }, GfxError);
}

TEST(ShaderReaderTest, ReadStage_ThrowsOnAStreamThatCannotBeRead)
{
    std::istringstream inputStream("#version 330\n");

    inputStream.setstate(std::ios::badbit);

    EXPECT_THROW({ (void)ShaderReader{}.readAll(inputStream); }, GfxError);
}

TEST(ShaderReaderTest, Read_PairsTheTwoStages)
{
    std::istringstream vertex("vertex text");
    std::istringstream fragment("fragment text");

    const ShaderSource source = ShaderReader{}.read(vertex, fragment);

    EXPECT_EQ("vertex text", source.vertex);
    EXPECT_EQ("fragment text", source.fragment);
    EXPECT_TRUE(source.isComplete());
}

TEST(ShaderReaderTest, Read_ThrowsWhenAStageIsEmpty)
{
    std::istringstream vertex("vertex text");
    std::istringstream fragment("");

    EXPECT_THROW(
        { (void)ShaderReader{}.read(vertex, fragment); }, GfxError);
}
