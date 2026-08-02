#include "antwika/replay/DocumentDepth.hpp"

#include <cstddef>
#include <utility>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

using antwika::replay::kMaxDocumentDepth;
using antwika::replay::nestsTooDeep;

namespace
{
    // A number under the given count of nested arrays.
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
} // namespace

TEST(DocumentDepthTest, APrimitiveIsNeverTooDeep)
{
    EXPECT_FALSE(nestsTooDeep(nlohmann::json(7)));
}

TEST(DocumentDepthTest, AllowsNestingUpToTheBound)
{
    EXPECT_FALSE(nestsTooDeep(nested(kMaxDocumentDepth)));
}

TEST(DocumentDepthTest, RefusesNestingJustPastTheBound)
{
    EXPECT_TRUE(nestsTooDeep(nested(kMaxDocumentDepth + 1)));
}

// An object's members sit a level below it, exactly as elements do.
TEST(DocumentDepthTest, ObjectsCountLikeArrays)
{
    nlohmann::json document;
    document["payload"] = nested(kMaxDocumentDepth);

    EXPECT_TRUE(nestsTooDeep(document));
}
