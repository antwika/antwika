#pragma once

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unistd.h>


namespace antwika::testing
{

    [[nodiscard]] inline std::filesystem::path scratchPath(
        std::string_view prefix)
    {
        const auto *const running
            = ::testing::UnitTest::GetInstance()->current_test_info();

        auto name = std::string{prefix};
        name += running->name();
        name += '.';
        name += std::to_string(::getpid());

        return std::filesystem::temp_directory_path() / name;
    }

}
