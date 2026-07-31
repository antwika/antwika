#include "antwika/ttf/TtfReader.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/ttf/Font.hpp"
#include "antwika/ttf/TtfError.hpp"

#include "SyntheticFont.hpp"

using antwika::ttf::Font;
using antwika::ttf::TtfError;
using antwika::ttf::TtfReader;
using antwika::ttf::tests::buildFont;

namespace
{
    std::istringstream streamOf(
        const std::vector<std::uint8_t> &bytes)
    {
        return std::istringstream(std::string(
            bytes.begin(), bytes.end()));
    }
} // namespace

TEST(TtfReaderTest, Read_ParsesAFontOffAStream)
{
    std::istringstream stream = streamOf(buildFont());
    const TtfReader reader;
    const Font font = reader.read(stream);

    EXPECT_TRUE(font.has(U'A'));
}

// The library opens no files.
// So an empty stream is ordinary input rather than a missing file.
TEST(TtfReaderTest, Read_RefusesAnEmptyStream)
{
    std::istringstream stream;
    const TtfReader reader;

    EXPECT_THROW((void)reader.read(stream), TtfError);
}
