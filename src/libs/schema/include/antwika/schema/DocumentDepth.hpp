#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstddef>

namespace antwika::schema
{

    inline constexpr std::size_t kMaxDocumentDepth = 16;

    [[nodiscard]] bool exceedsMaxDepth(const nlohmann::json &document);

}
