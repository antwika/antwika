#pragma once

#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include <antwika/io/File.hpp>

namespace antwika::app
{

    /**
     * @brief A snapshot kept in one file beside the working directory.
     *
     * Two applications wrote this class out in full -- `companion`'s
     * `FilePetStore` and `tower_defence`'s `FileScoreStore` -- and the
     * two differed in seventeen lines, every one of them a type name or
     * a message string.
     * What they share is the whole of it: a missing file is a first
     * run rather than a malformed one, and a write that cannot open or
     * cannot land says so -- through antwika::io, which owns that
     * discipline for every module that touches a file.
     *
     * The reader and the writer arrive as plain function pointers, so
     * a store holds no state beyond its path and an application's own
     * format functions stay in that application.
     *
     * @tparam ValueT What the file holds.
     * @tparam ErrorT What a failed write is reported as; the format's
     * own type, since the house rule is one exception type per failure
     * category.
     */
    template <typename ValueT, typename ErrorT>
    class FileSnapshotStore
    {
    public:
        /** @brief Reads a value off a stream that is known to be open. */
        using Reader = ValueT (*)(std::istream &);

        /** @brief Writes a value onto an open stream. */
        using Writer = void (*)(const ValueT &, std::ostream &);

        /**
         * @brief Keep a snapshot at a path.
         * @param path Where the file is; it need not exist yet.
         * @param read What decodes the file.
         * @param write What encodes it.
         * @param subject What to call the thing in a failure message,
         * as an application would name it -- "a companion", "a high
         * score".
         */
        FileSnapshotStore(
            std::string path,
            Reader read,
            Writer write,
            std::string subject)
            : path(std::move(path)),
              read(read),
              write(write),
              subject(std::move(subject))
        {
        }

        /**
         * @brief Read what the file holds.
         *
         * **A file that is not there is not a malformed one.** It is a
         * first run, and unchecked it would reach the parser as an
         * empty stream, which reports "you have never had one of these"
         * as corruption.
         *
         * @return What the file held, or nothing when it is not there.
         * @throws ErrorT If a file is there and cannot be read.
         */
        [[nodiscard]] std::optional<ValueT> loadIfPresent() const
        {
            auto file = io::openToReadIfPresent(path);

            if (!file.has_value())
            {
                return std::nullopt;
            }

            return read(*file);
        }

        /**
         * @brief Write a value out, replacing whatever was there.
         *
         * The open, the flush and the failed-write refusal are
         * antwika::io's, stated once over there.
         *
         * @param value What to write.
         * @throws ErrorT If the file cannot be opened or written.
         */
        void store(const ValueT &value) const
        {
            io::writeFileAs<ErrorT>(
                path, subject, [this, &value](std::ostream &out) {
                    write(value, out);
                });
        }

    private:
        std::string path;
        Reader read;
        Writer write;
        std::string subject;
    };

} // namespace antwika::app
