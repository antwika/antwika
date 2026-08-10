#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstddef>

namespace antwika::replay
{

    inline constexpr std::size_t kMaxDocumentDepth = 16;

    [[nodiscard]] bool nestsTooDeep(const nlohmann::json &document);

}
