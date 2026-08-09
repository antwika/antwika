#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <utility>

#include "antwika/replay/DocumentDepth.hpp"

using antwika::replay::kMaxDocumentDepth;
using antwika::replay::nestsTooDeep;

namespace
{
    [[nodiscard]] nlohmann::json nested(const std::size_t levels)
    {
        nlohmann::json value = 7;

        for (std::size_t level = 0; level < levels; ++level)
        {
            nlohmann::json wrapped = nlohmann::json::array();
            wrapped.push_back(std::move(value));
            value = std::move(wrapped);
        }

        return value;
    }
}

TEST(DocumentDepthTest, MaxDocumentDepth_IsSixteenLevels)
{
    EXPECT_EQ(kMaxDocumentDepth, 16U);
}

TEST(DocumentDepthTest, NestsTooDeep_IsFalseForAPrimitive)
{
    EXPECT_FALSE(nestsTooDeep(nlohmann::json(7)));
}

TEST(DocumentDepthTest, NestsTooDeep_AllowsNestingUpToTheBound)
{
    EXPECT_FALSE(nestsTooDeep(nested(kMaxDocumentDepth)));
}

TEST(DocumentDepthTest, NestsTooDeep_IsTrueJustPastTheBound)
{
    EXPECT_TRUE(nestsTooDeep(nested(kMaxDocumentDepth + 1)));
}

TEST(DocumentDepthTest, NestsTooDeep_CountsObjectsLikeArrays)
{
    nlohmann::json document;
    document["payload"] = nested(kMaxDocumentDepth);

    EXPECT_TRUE(nestsTooDeep(document));
}
