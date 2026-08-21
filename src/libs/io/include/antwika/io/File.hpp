#pragma once

#include <cstdint>
#include <fstream>
#include <ios>
#include <optional>
#include <ostream>
#include <string>

namespace antwika::io
{

    enum class ContentKind : std::uint8_t
    {
        Text,
        Bytes
    };

    namespace detail
    {
        [[nodiscard]] inline std::ios_base::openmode readMode(
            ContentKind content) noexcept
        {
            return content == ContentKind::Bytes
                ? std::ios_base::in | std::ios_base::binary
                : std::ios_base::in;
        }

        [[nodiscard]] inline std::ios_base::openmode writeMode(
            ContentKind content) noexcept
        {
            return content == ContentKind::Bytes
                ? std::ios_base::out | std::ios_base::binary
                : std::ios_base::out;
        }
    }

    [[nodiscard]] inline std::optional<std::ifstream> openToReadIfPresent(
        const std::string &path, ContentKind content = ContentKind::Text)
    {
        std::ifstream file(path, detail::readMode(content));

        if (!file.is_open())
        {
            return std::nullopt;
        }

        return file;
    }

    template <typename ErrorT>
    [[nodiscard]] std::ifstream openToReadAs(
        const std::string &path,
        const std::string &description,
        ContentKind content = ContentKind::Text)
    {
        std::ifstream file(path, detail::readMode(content));

        if (!file.is_open())
        {
            throw ErrorT(
                "antwika: could not open " + description + " to read: "
                + path);
        }

        return file;
    }

    template <typename ErrorT>
    [[nodiscard]] std::ofstream openToWriteAs(
        const std::string &path,
        const std::string &description,
        ContentKind content = ContentKind::Text)
    {
        std::ofstream file(path, detail::writeMode(content));

        if (!file.is_open())
        {
            throw ErrorT(
                "antwika: could not open " + description + " to write: "
                + path);
        }

        return file;
    }

    template <typename ErrorT>
    void requireStreamOkAs(
        std::ostream &outputStream,
        const std::string &description,
        const std::string &destination)
    {
        outputStream.flush();

        if (!outputStream)
        {
            throw ErrorT(
                "antwika: could not write " + description + ": "
                + destination);
        }
    }

    template <typename ErrorT, typename WriteBody>
    void writeFileAs(
        const std::string &path,
        const std::string &description,
        WriteBody &&write,
        ContentKind content = ContentKind::Text)
    {
        std::ofstream file =
            openToWriteAs<ErrorT>(path, description, content);

        write(file);

        requireStreamOkAs<ErrorT>(file, description, path);
    }

}
