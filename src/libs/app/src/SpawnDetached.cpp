#include "antwika/app/SpawnDetached.hpp"

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>

#include <unistd.h>
#endif

namespace antwika::app
{

#ifdef _WIN32

    namespace
    {
        [[nodiscard]] std::string quotedForCommandLine(
            const std::string &argument)
        {
            if (!argument.empty() &&
                argument.find_first_of(" \t\"") == std::string::npos)
            {
                return argument;
            }

            std::string quoted = "\"";
            std::size_t backslashes = 0;

            for (const auto character : argument)
            {
                if (character == '\\')
                {
                    ++backslashes;
                    continue;
                }

                quoted.append(
                    character == '"' ? backslashes * 2 : backslashes, '\\');
                backslashes = 0;

                if (character == '"')
                {
                    quoted.push_back('\\');
                }

                quoted.push_back(character);
            }

            quoted.append(backslashes * 2, '\\');
            quoted.push_back('"');

            return quoted;
        }
    }

    bool spawnDetached(
        const std::string &program,
        const std::vector<std::string> &arguments)
    {
        auto commandLine = quotedForCommandLine(program);

        for (const auto &argument : arguments)
        {
            commandLine.push_back(' ');
            commandLine.append(quotedForCommandLine(argument));
        }

        STARTUPINFOA startupInfo{};

        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInformation{};

        const auto started = ::CreateProcessA(
            program.c_str(),
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
            nullptr,
            nullptr,
            &startupInfo,
            &processInformation);

        if (started == FALSE)
        {
            return false;
        }

        ::CloseHandle(processInformation.hThread);
        ::CloseHandle(processInformation.hProcess);

        return true;
    }

#else

    bool spawnDetached(
        const std::string &program,
        const std::vector<std::string> &arguments)
    {
        if (access(program.c_str(), X_OK) != 0)
        {
            return false;
        }

        std::vector<char *> argv;

        argv.push_back(const_cast<char *>(program.c_str()));

        for (const auto &argument : arguments)
        {
            argv.push_back(const_cast<char *>(argument.c_str()));
        }

        argv.push_back(nullptr);

        const auto first = fork();

        if (first < 0)
        {
            return false;
        }

        if (first == 0)
        {
            setsid();

            if (fork() == 0)
            {
                execv(program.c_str(), argv.data());
                _exit(127);
            }

            _exit(0);
        }

        int status = 0;

        waitpid(first, &status, 0);

        return true;
    }

#endif

}
