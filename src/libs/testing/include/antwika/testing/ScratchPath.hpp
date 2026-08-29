#pragma once

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace antwika::testing
{

    [[nodiscard]] inline int getProcessId()
    {
#ifdef _WIN32
        return ::_getpid();
#else
        return ::getpid();
#endif
    }

    [[nodiscard]] inline std::string getScratchName(
        std::string_view prefix,
        std::string_view suiteName,
        std::string_view testName)
    {
        auto name = std::string{prefix};
        name += '.';
        name += suiteName;
        name += '.';
        name += testName;
        name += '.';
        name += std::to_string(getProcessId());

        std::ranges::replace(name, '/', '-');

        return name;
    }

    [[nodiscard]] inline std::filesystem::path getScratchPath(
        std::string_view prefix)
    {
        const auto *const running
            = ::testing::UnitTest::GetInstance()->current_test_info();

        if (running == nullptr)
        {
            return std::filesystem::temp_directory_path()
                / getScratchName(prefix, "no-suite", "no-test");
        }

        return std::filesystem::temp_directory_path()
            / getScratchName(
                prefix, running->test_suite_name(), running->name());
    }

}
