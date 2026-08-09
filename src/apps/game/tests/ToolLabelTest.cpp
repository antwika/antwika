#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <set>
#include <string_view>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageSet.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/Toolbar.hpp"

using antwika::game::BuildTool;
using antwika::game::kBuildToolCount;
using antwika::game::MessageId;
using antwika::game::Messages;
using antwika::game::pauseLabel;
using antwika::game::toolLabel;
using antwika::i18n::Locale;
using antwika::i18n::nameOf;

namespace
{
    constexpr std::array<Locale, 2> kEveryLocale{
        Locale::English, Locale::Swedish};

    void expectWordedEverywhere(MessageId id)
    {
        for (const auto locale : kEveryLocale)
        {
            const auto text = Messages::catalogueFor(locale).find(id);

            ASSERT_TRUE(text.has_value()) << nameOf<Messages>(id);
            EXPECT_FALSE(text->empty()) << nameOf<Messages>(id);
        }
    }
}

TEST(ToolLabelTest, ToolLabel_IsUniquePerTool)
{
    std::set<MessageId> labels;

    for (std::size_t index = 0; index < kBuildToolCount; ++index)
    {
        labels.insert(toolLabel(static_cast<BuildTool>(index)));
    }

    EXPECT_EQ(labels.size(), kBuildToolCount);
}

TEST(ToolLabelTest, ToolLabel_IsInBothCatalogues)
{
    for (std::size_t index = 0; index < kBuildToolCount; ++index)
    {
        expectWordedEverywhere(toolLabel(static_cast<BuildTool>(index)));
    }
}

TEST(ToolLabelTest, PauseLabel_SaysWhatPressingWouldDo)
{
    EXPECT_NE(pauseLabel(true), pauseLabel(false));

    expectWordedEverywhere(pauseLabel(true));
    expectWordedEverywhere(pauseLabel(false));
}

TEST(ToolLabelTest, ToolLabel_IsDistinctWithinALocale)
{
    for (const auto locale : kEveryLocale)
    {
        std::set<std::string_view> words;

        for (std::size_t index = 0; index < kBuildToolCount; ++index)
        {
            const auto text = Messages::catalogueFor(locale).find(
                toolLabel(static_cast<BuildTool>(index)));

            ASSERT_TRUE(text.has_value());
            words.insert(*text);
        }

        EXPECT_EQ(words.size(), kBuildToolCount);
    }
}
