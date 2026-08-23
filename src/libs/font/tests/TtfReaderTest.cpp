#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "antwika/font/TtfReader.hpp"
#include "antwika/font/Font.hpp"
#include "antwika/font/FontError.hpp"
#include "SyntheticFont.hpp"

using antwika::font::Font;
using antwika::font::FontError;
using antwika::font::TtfReader;
using antwika::font::tests::createFont;

namespace
{
    std::istringstream streamOf(
        const std::vector<std::uint8_t> &bytes)
    {
        return std::istringstream(std::string(
            bytes.begin(), bytes.end()));
    }
}

TEST(TtfReaderTest, Read_ParsesAFontOffAStream)
{
    std::istringstream stream = streamOf(createFont());
    const TtfReader reader;
    const Font font = reader.read(stream);

    EXPECT_TRUE(font.has(U'A'));
}

TEST(TtfReaderTest, Read_RefusesAnEmptyStream)
{
    std::istringstream stream;
    const TtfReader reader;

    EXPECT_THROW((void)reader.read(stream), FontError);
}
