#pragma once

#include <cstdint>
#include <fstream>
#include <ios>
#include <optional>
#include <ostream>
#include <string>

namespace antwika::io
{

    enum class Content : std::uint8_t
    {
        Text,
        Bytes
    };

    namespace detail
    {
        [[nodiscard]] inline std::ios_base::openmode readMode(
            Content content) noexcept
        {
            return content == Content::Bytes
                ? std::ios_base::in | std::ios_base::binary
                : std::ios_base::in;
        }

        [[nodiscard]] inline std::ios_base::openmode writeMode(
            Content content) noexcept
        {
            return content == Content::Bytes
                ? std::ios_base::out | std::ios_base::binary
                : std::ios_base::out;
        }
    }

    [[nodiscard]] inline std::optional<std::ifstream> openToReadIfPresent(
        const std::string &path, Content content = Content::Text)
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
        const std::string &subject,
        Content content = Content::Text)
    {
        std::ifstream file(path, detail::readMode(content));

        if (!file.is_open())
        {
            throw ErrorT(
                "antwika: could not open " + subject + " to read: "
                + path);
        }

        return file;
    }

    template <typename ErrorT>
    [[nodiscard]] std::ofstream openToWriteAs(
        const std::string &path,
        const std::string &subject,
        Content content = Content::Text)
    {
        std::ofstream file(path, detail::writeMode(content));

        if (!file.is_open())
        {
            throw ErrorT(
                "antwika: could not open " + subject + " to write: "
                + path);
        }

        return file;
    }

    template <typename ErrorT>
    void requireStreamTookAs(
        std::ostream &out,
        const std::string &subject,
        const std::string &destination)
    {
        out.flush();

        if (!out)
        {
            throw ErrorT(
                "antwika: could not write " + subject + ": "
                + destination);
        }
    }

    template <typename ErrorT, typename WriteBody>
    void writeFileAs(
        const std::string &path,
        const std::string &subject,
        WriteBody &&write,
        Content content = Content::Text)
    {
        std::ofstream file =
            openToWriteAs<ErrorT>(path, subject, content);

        write(file);

        requireStreamTookAs<ErrorT>(file, subject, path);
    }

}
