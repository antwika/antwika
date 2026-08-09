#pragma once

#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <utility>

#include <antwika/io/File.hpp>

namespace antwika::app
{

    template <typename ValueT, typename ErrorT>
    class FileSnapshotStore final
    {
    public:
        using Reader = ValueT (*)(std::istream &);

        using Writer = void (*)(const ValueT &, std::ostream &);

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

        [[nodiscard]] std::optional<ValueT> loadIfPresent() const
        {
            auto file = io::openToReadIfPresent(path);

            if (!file.has_value())
            {
                return std::nullopt;
            }

            return read(*file);
        }

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

}
