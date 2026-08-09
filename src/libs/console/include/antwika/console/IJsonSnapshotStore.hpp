#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>

#include <antwika/config/Format.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/console/ISnapshotStore.hpp"
#include "antwika/console/SnapshotError.hpp"
#include "antwika/console/SnapshotFormat.hpp"

namespace antwika::console
{

    template <typename ErrorT>
    class IJsonSnapshotStore : public ISnapshotStore
    {
    public:
        IJsonSnapshotStore(
            antwika::config::Format format,
            std::string_view title,
            antwika::replay::MigrationChain (*migrations)())
            : format(format, title, migrations)
        {
        }

        IJsonSnapshotStore(const IJsonSnapshotStore &) = delete;
        IJsonSnapshotStore(IJsonSnapshotStore &&) = delete;

        IJsonSnapshotStore &operator=(const IJsonSnapshotStore &) =
            delete;
        IJsonSnapshotStore &operator=(IJsonSnapshotStore &&) = delete;

        void dump(
            const std::string &path,
            const std::vector<std::string> &console) final
        {
            Snapshot snapshot;
            snapshot.console = console;

            try
            {
                snapshot.state = takeState(path);
            }
            catch (const ErrorT &failed)
            {
                throw SnapshotError(failed.what());
            }

            format.write(snapshot, path);
        }

        [[nodiscard]] std::vector<std::string> load(
            const std::string &path) final
        {
            auto snapshot = format.read(path);

            try
            {
                applyState(path, snapshot.state);
            }
            catch (const ErrorT &failed)
            {
                throw SnapshotError(failed.what());
            }

            return snapshot.console;
        }

    protected:
        [[nodiscard]] virtual nlohmann::json takeState(
            const std::string &path) = 0;

        virtual void applyState(
            const std::string &path, const nlohmann::json &state) = 0;

    private:
        SnapshotFormat format;
    };

}
