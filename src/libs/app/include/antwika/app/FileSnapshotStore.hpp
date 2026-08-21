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
            Reader reader,
            Writer writer,
            std::string description)
            : path(std::move(path)),
              read(reader),
              write(writer),
              description(std::move(description))
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
                path, description, [this, &value](std::ostream &outputStream) {
                    write(value, outputStream);
                });
        }

    private:
        std::string path;
        Reader reader;
        Writer writer;
        std::string description;
    };

}
