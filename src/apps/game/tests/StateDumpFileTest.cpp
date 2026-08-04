#include <gtest/gtest.h>

#include <fstream>

#include <antwika/testing/ScratchPath.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/StateDump.hpp"
#include "antwika/game/StateDumpFile.hpp"

using antwika::game::BuildTool;
using antwika::game::loadStateDump;
using antwika::game::SaveFormatError;
using antwika::game::StateDump;
using antwika::game::stateDumpFile;

namespace
{
    [[nodiscard]] StateDump smallDump()
    {
        StateDump dump;

        dump.save.paths = {{.x = 1, .y = 2}};
        dump.paused = true;
        dump.tool = BuildTool::Road;
        dump.console = {"> dump_state"};

        return dump;
    }
} // namespace

TEST(StateDumpFileTest, RoundTrip_AFileComesBackAsItWent)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_state_dump.json");
    const auto dump = smallDump();

    stateDumpFile(dump, file.path().string());

    EXPECT_EQ(loadStateDump(file.path().string()), dump);
}

TEST(StateDumpFileTest, Load_RefusesAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_state_dump_absent.json");

    EXPECT_THROW(
        (void)loadStateDump(file.path().string()), SaveFormatError);
}

TEST(StateDumpFileTest, Load_RefusesAFileThatIsNotJson)
{
    const antwika::testing::ScratchFile file(
        "antwika_game_state_dump_torn.json");

    {
        std::ofstream out(file.path());
        out << "{ not json";
    }

    EXPECT_THROW(
        (void)loadStateDump(file.path().string()), SaveFormatError);
}
