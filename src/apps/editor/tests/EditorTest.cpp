#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/editor/Editor.hpp"

using antwika::editor::Editor;
using antwika::gfx::NullBackend;
using antwika::input::NullInputBackend;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    class EditorTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        NullBackend backend{logger};
        NullInputBackend inputs{logger};
    };

}

TEST_F(EditorTest, Editor_OpensAgainstABackendItIsHanded)
{
    EXPECT_NO_THROW({
        const Editor editor(
            logger, backend, inputs, std::string(kMissingMapPath));
    });
}

TEST_F(EditorTest, Editor_StartsFromTheBuiltInMapWhenThereIsNoneToLoad)
{
    EXPECT_CALL(logger, log(_, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(
        logger,
        log(Level::Info, HasSubstr("starting from the built-in one")));

    const Editor editor(
        logger, backend, inputs, std::string(kMissingMapPath));
}

TEST_F(EditorTest, Editor_OpensStraightIntoPlayWhenItIsAskedTo)
{
    EXPECT_NO_THROW({
        const Editor editor(
            logger, backend, inputs, std::string(kMissingMapPath), true);
    });
}
