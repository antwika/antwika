#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <utility>

#include "antwika/schema/DocumentDepth.hpp"

using antwika::schema::kMaxDocumentDepth;
using antwika::schema::exceedsMaxDepth;

namespace
{
    [[nodiscard]] nlohmann::json getNestedDocument(const std::size_t levels)
    {
        nlohmann::json value = 7;

        for (std::size_t level = 0; level < levels; ++level)
        {
            nlohmann::json wrappedJson = nlohmann::json::array();
            wrappedJson.push_back(std::move(value));
            value = std::move(wrappedJson);
        }

        return value;
    }
}

TEST(DocumentDepthTest, MaxDocumentDepth_IsSixteenLevels)
{
    EXPECT_EQ(kMaxDocumentDepth, 16U);
}

TEST(DocumentDepthTest, ExceedsMaxDepth_IsFalseForAPrimitive)
{
    EXPECT_FALSE(exceedsMaxDepth(nlohmann::json(7)));
}

TEST(DocumentDepthTest, ExceedsMaxDepth_AllowsNestingUpToTheBound)
{
    EXPECT_FALSE(exceedsMaxDepth(getNestedDocument(kMaxDocumentDepth)));
}

TEST(DocumentDepthTest, ExceedsMaxDepth_IsTrueJustPastTheBound)
{
    EXPECT_TRUE(exceedsMaxDepth(getNestedDocument(kMaxDocumentDepth + 1)));
}

TEST(DocumentDepthTest, ExceedsMaxDepth_CountsObjectsLikeArrays)
{
    nlohmann::json document;
    document["payload"] = getNestedDocument(kMaxDocumentDepth);

    EXPECT_TRUE(exceedsMaxDepth(document));
}
