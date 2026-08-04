#pragma once

#include <fstream>
#include <ios>
#include <optional>
#include <ostream>
#include <string>

namespace antwika::io
{

    /**
     * @brief Whether a file holds text or raw bytes.
     *
     * Bytes opens the stream in binary mode, which matters on the
     * MinGW build: a text-mode stream there rewrites every newline on
     * the way through, and a PNG with its newlines rewritten is not a
     * PNG any more.
     */
    enum class Content
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
    } // namespace detail

    /**
     * @brief Open a file to read, where an absent file is a state.
     *
     * **A file that is not there is not a malformed one.** For a
     * config, a save or a snapshot it is a first run, and unchecked it
     * would reach a parser as an empty stream, which reports "you have
     * never had one of these" as corruption.
     * A caller for whom absence *is* a failure uses openToReadAs()
     * instead.
     *
     * @param path Where the file would be.
     * @param content Text unless the caller says Bytes.
     * @return The open stream, or nothing when the file is not there.
     */
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

    /**
     * @brief Open a file to read, where an absent file is a failure.
     * @tparam ErrorT What the caller reports the refusal as.
     * @param path Where the file should be.
     * @param subject What to call the thing in the failure message, as
     * the caller would name it -- "a replay", "a save".
     * @param content Text unless the caller says Bytes.
     * @return The open stream.
     * @throws ErrorT If there is nothing to open at the path.
     */
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

    /**
     * @brief Open a file to write, replacing whatever was there.
     *
     * Opening is not writing: a stream this hands back can still fail
     * on the way out, which is what requireStreamTookAs() is for, and
     * what writeFileAs() says in one call for the whole-file case.
     *
     * @tparam ErrorT What the caller reports the refusal as.
     * @param path Where to write.
     * @param subject What to call the thing in the failure message.
     * @param content Text unless the caller says Bytes.
     * @return The open stream.
     * @throws ErrorT If the path cannot be opened to write.
     */
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

    /**
     * @brief Flush a stream and refuse one that did not take it all.
     *
     * **The flush happens here rather than in a destructor, because a
     * destructor cannot say that it failed.** A full disk fails on the
     * flush and not on the open, so a write nobody flushed and checked
     * is one the filesystem may refuse in silence -- and a save that
     * loses its document quietly is the one failure a program cannot
     * recover from.
     *
     * @tparam ErrorT What the caller reports the loss as.
     * @param out The stream everything was written onto.
     * @param subject What to call the thing in the failure message.
     * @param destination Where the write was going, for the message.
     * @throws ErrorT If the stream did not take everything.
     */
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

    /**
     * @brief Write a whole file: open, write, flush, and check.
     *
     * The one call for the common case, so a module writing a document
     * states its format and nothing else -- the open refusal, the
     * flush and the failed-write refusal are this library's to get
     * right, once.
     *
     * @tparam ErrorT What the caller reports either refusal as.
     * @param path Where to write.
     * @param subject What to call the thing in a failure message.
     * @param write Called with the open stream to put the content on.
     * @param content Text unless the caller says Bytes.
     * @throws ErrorT If the file cannot be opened or written.
     */
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

} // namespace antwika::io
