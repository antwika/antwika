#include <gtest/gtest.h>

#include <filesystem>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>

#include <antwika/io/File.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/testing/ScratchFile.hpp>

using antwika::io::ContentKind;
using antwika::io::openToReadAs;
using antwika::io::openToReadIfPresent;
using antwika::io::openToWriteAs;
using antwika::io::requireStreamOkAs;
using antwika::io::writeFileAs;
using antwika::testing::ScratchFile;

namespace
{
    class TestError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    [[nodiscard]] std::string readWholeFile(std::istream &inputStream)
    {
        std::ostringstream all;
        all << inputStream.rdbuf();
        return all.str();
    }
}

TEST(FileTest, ReadMode_AsksForBinaryOnlyForBytes)
{
    EXPECT_EQ(
        antwika::io::detail::readMode(ContentKind::Bytes),
        std::ios_base::in | std::ios_base::binary);
    EXPECT_EQ(
        antwika::io::detail::readMode(ContentKind::Text), std::ios_base::in);
}

TEST(FileTest, WriteMode_AsksForBinaryOnlyForBytes)
{
    EXPECT_EQ(
        antwika::io::detail::writeMode(ContentKind::Bytes),
        std::ios_base::out | std::ios_base::binary);
    EXPECT_EQ(
        antwika::io::detail::writeMode(ContentKind::Text),
        std::ios_base::out);
}

TEST(FileTest, OpenToReadIfPresent_AnswersNothingForAnAbsentFile)
{
    const ScratchFile file("antwika_io_absent.txt");

    EXPECT_FALSE(openToReadIfPresent(file.string()).has_value());
}

TEST(FileTest, OpenToReadIfPresent_OpensWhatIsThere)
{
    const ScratchFile file("antwika_io_present.txt");
    file.write("present");

    auto openedStream = openToReadIfPresent(file.string());

    ASSERT_TRUE(openedStream.has_value());
    EXPECT_EQ("present", readWholeFile(*openedStream));
}

TEST(FileTest, OpenToReadAs_RefusesAnAbsentFile)
{
    const ScratchFile file("antwika_io_required.txt");

    try
    {
        (void)openToReadAs<TestError>(file.string(), "a fixture");
        FAIL() << "an absent file was opened";
    }
    catch (const TestError &error)
    {
        const std::string message = error.what();

        EXPECT_NE(message.find("could not open"), std::string::npos);
        EXPECT_NE(message.find("to read"), std::string::npos);
        EXPECT_NE(message.find("a fixture"), std::string::npos);
        EXPECT_NE(message.find(file.string()), std::string::npos);
    }
}

TEST(FileTest, OpenToReadAs_OpensWhatIsThere)
{
    const ScratchFile file("antwika_io_read.txt");
    file.write("held");

    auto openedStream = openToReadAs<TestError>(file.string(), "a fixture");

    EXPECT_EQ("held", readWholeFile(openedStream));
}

TEST(FileTest, OpenToWriteAs_RefusesAPathThatCannotBeOpened)
{
    try
    {
        (void)openToWriteAs<TestError>(
            "/nonexistent-directory/file.txt", "a fixture");
        FAIL() << "an unwritable path was opened";
    }
    catch (const TestError &error)
    {
        const std::string message = error.what();

        EXPECT_NE(message.find("could not open"), std::string::npos);
        EXPECT_NE(message.find("to write"), std::string::npos);
        EXPECT_NE(message.find("a fixture"), std::string::npos);
        EXPECT_NE(
            message.find("/nonexistent-directory/file.txt"),
            std::string::npos);
    }
}

TEST(FileTest, RequireStreamOkAs_AcceptsAStreamThatTookIt)
{
    std::ostringstream outputStream;
    outputStream << "taken";

    EXPECT_NO_THROW(
        requireStreamOkAs<TestError>(outputStream, "a fixture", "somewhere"));
}

TEST(FileTest, RequireStreamOkAs_RefusesAStreamThatFailed)
{
    std::ostringstream outputStream;
    outputStream.setstate(std::ios_base::badbit);

    try
    {
        requireStreamOkAs<TestError>(outputStream, "a fixture", "somewhere");
        FAIL() << "a failed stream was accepted";
    }
    catch (const TestError &error)
    {
        const std::string message = error.what();

        EXPECT_NE(message.find("could not write"), std::string::npos);
        EXPECT_NE(message.find("a fixture"), std::string::npos);
        EXPECT_NE(message.find("somewhere"), std::string::npos);
    }
}

TEST(FileTest, WriteFileAs_WritesWhatTheBodyPutsOn)
{
    const ScratchFile file("antwika_io_written.txt");

    writeFileAs<TestError>(
        file.string(), "a fixture", [](std::ostream &outputStream) {
            outputStream << "whole";
        });

    auto openedStream = openToReadIfPresent(file.string());

    ASSERT_TRUE(openedStream.has_value());
    EXPECT_EQ("whole", readWholeFile(*openedStream));
}

TEST(FileTest, WriteFileAs_RefusesAPathThatCannotBeOpened)
{
    EXPECT_THROW(
        writeFileAs<TestError>(
            "/nonexistent-directory/file.txt",
            "a fixture",
            [](std::ostream &outputStream) { outputStream << "lost"; }),
        TestError);
}

TEST(FileTest, WriteFileAs_ReportsAWriteThatFailsAfterTheOpen)
{
    if (!std::filesystem::exists("/dev/full"))
    {
        GTEST_SKIP() << "no /dev/full to fill";
    }

    EXPECT_THROW(
        writeFileAs<TestError>(
            "/dev/full",
            "a fixture",
            [](std::ostream &outputStream) { outputStream << "swallowed"; }),
        TestError);
}

TEST(FileTest, WriteFileAs_CarriesBytesThroughUntouched)
{
    const ScratchFile file("antwika_io_bytes.bin");
    const std::string bytes("\r\n\0\n", 4);

    writeFileAs<TestError>(
        file.string(),
        "a fixture",
        [&bytes](std::ostream &outputStream) {
            outputStream.write(bytes.data(),
                      static_cast<std::streamsize>(bytes.size()));
        },
        ContentKind::Bytes);

    auto openedStream = openToReadAs<TestError>(
        file.string(), "a fixture", ContentKind::Bytes);

    EXPECT_EQ(bytes, readWholeFile(openedStream));
}
