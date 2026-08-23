#pragma once

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "antwika/editor/plan/PlanBoard.hpp"

namespace antwika::editor
{

    inline constexpr std::string_view kPlanMagic = "antwika.plan";

    inline constexpr std::uint32_t kPlanVersion = 1;

    inline constexpr std::string_view kDefaultPlanPath =
        "assets/plan.json";

    void writeBoard(std::ostream &outputStream, const Board &board);

    [[nodiscard]] Board readBoard(std::istream &inputStream);

    void saveBoard(const std::string &path, const Board &board);

    [[nodiscard]] std::optional<Board> getLoadBoard(
        const std::string &path);

}
