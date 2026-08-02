#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <set>
#include <string_view>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Toolbar.hpp"

using antwika::game::BuildTool;
using antwika::game::kBuildToolCount;
using antwika::game::pauseLabel;
using antwika::game::toolLabel;
using antwika::i18n::catalogueFor;
using antwika::i18n::Locale;
using antwika::i18n::MessageId;
using antwika::i18n::nameOf;

namespace
{
    // Both of them, so a forgotten Swedish entry is a red test.
    // Rather than an English caption in a Swedish window.
    constexpr std::array<Locale, 2> kEveryLocale{
        Locale::English, Locale::Swedish};

    void expectWordedEverywhere(MessageId id)
    {
        for (const auto locale : kEveryLocale)
        {
            const auto text = catalogueFor(locale).find(id);

            ASSERT_TRUE(text.has_value())
                << nameOf(id) << " is missing a translation";
            EXPECT_FALSE(text->empty()) << nameOf(id);
        }
    }
} // namespace

// Two tools sharing a caption is two buttons reading the same.
// A player would then have no way to tell which places what.
TEST(ToolLabelTest, EveryToolIsLabelledWithAnIdOfItsOwn)
{
    std::set<MessageId> labels;

    for (std::size_t index = 0; index < kBuildToolCount; ++index)
    {
        labels.insert(toolLabel(static_cast<BuildTool>(index)));
    }

    EXPECT_EQ(labels.size(), kBuildToolCount);
}

// A caption goes through antwika::i18n.
// So it exists only as far as both catalogues carry it.
TEST(ToolLabelTest, EveryToolsLabelIsInBothCatalogues)
{
    for (std::size_t index = 0; index < kBuildToolCount; ++index)
    {
        expectWordedEverywhere(toolLabel(static_cast<BuildTool>(index)));
    }
}

// The button says what pressing it does, not what state it is in.
// Which is only true while the two ids differ.
TEST(ToolLabelTest, ThePauseButtonSaysWhatPressingItWouldDo)
{
    EXPECT_NE(pauseLabel(true), pauseLabel(false));

    expectWordedEverywhere(pauseLabel(true));
    expectWordedEverywhere(pauseLabel(false));
}

// The two texts have to differ.
// Otherwise one language lays the bar out unlike the other.
// And a recorded click then resolves to a different button.
TEST(ToolLabelTest, EveryToolsLabelIsDistinctWithinEachLocale)
{
    for (const auto locale : kEveryLocale)
    {
        std::set<std::string_view> words;

        for (std::size_t index = 0; index < kBuildToolCount; ++index)
        {
            const auto text = catalogueFor(locale).find(
                toolLabel(static_cast<BuildTool>(index)));

            ASSERT_TRUE(text.has_value());
            words.insert(*text);
        }

        EXPECT_EQ(words.size(), kBuildToolCount);
    }
}
