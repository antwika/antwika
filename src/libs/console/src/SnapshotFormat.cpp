#include "antwika/console/SnapshotFormat.hpp"

namespace antwika::console
{

    namespace
    {
        void describeMembers(nlohmann::json &schema)
        {
            // The state is the application's own.
            // It validates behind ISnapshotStore.
            // The envelope only says that one is there.
            schema["required"] = {
                "magic", "console", "state"}; // GCOVR_EXCL_LINE
            schema["properties"]["console"]["type"] = "array";
            schema["properties"]["console"]["items"]["type"] = "string";
            schema["properties"]["state"]["type"] = "object";
        }

        void encodeMembers(
            const Snapshot &snapshot, nlohmann::json &encoded)
        {
            encoded["console"] = snapshot.console;
            encoded["state"] = snapshot.state;
        }

        Snapshot decodeMembers(const nlohmann::json &document)
        {
            Snapshot snapshot;

            snapshot.console = document.at("console")
                                   .get<std::vector<std::string>>();
            snapshot.state = document.at("state");

            return snapshot;

            // gcov puts the returned value's unwind block here.
            // SaveGame.cpp's own encoder explains it at length.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        // One line for every format, whichever application's it is.
        constexpr std::string_view kWhatFailed =
            "antwika::console: state dump JSON failed schema "
            "validation: ";
    } // namespace

    SnapshotFormat::SnapshotFormat(
        antwika::config::Format declared,
        std::string_view title,
        antwika::replay::MigrationChain (*migrations)())
        : format(antwika::config::FormatSpec<Snapshot>{
              .format = declared,
              .title = title,
              .whatFailed = kWhatFailed,
              .members = describeMembers,
              .encode = encodeMembers,
              .decode = decodeMembers,
              .migrations = migrations}) // GCOVR_EXCL_LINE
    {
    }

    nlohmann::json SnapshotFormat::toJson(const Snapshot &snapshot) const
    {
        return format.toJson(snapshot);
    }

    Snapshot SnapshotFormat::fromJson(
        const nlohmann::json &document) const
    {
        return format.fromJson(document);
    }

    void SnapshotFormat::write(
        const Snapshot &snapshot, const std::string &path) const
    {
        format.storeFile(snapshot, path);
    }

    Snapshot SnapshotFormat::read(const std::string &path) const
    {
        return format.loadFile(path);
    }

} // namespace antwika::console
