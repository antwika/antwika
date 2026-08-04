#include "antwika/music_editor/StateDump.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/sequencer/Rational.hpp>
#include <antwika/sequencer/SequencerError.hpp>
#include <antwika/sequencer/TempoMap.hpp>

#include "antwika/music_editor/StateDumpError.hpp"

namespace antwika::music_editor
{

    namespace
    {
        // The names a dump document holds, one per board.
        // Persisted, so they may not change once written.
        constexpr std::array<std::string_view, 2> kLayoutNames{
            "swedish", "english"};

        // The names a dump document holds, one per modal.
        constexpr std::array<std::string_view, 3> kModalNames{
            "none", "save", "load"};

        // The names a dump document holds, one per drag home.
        constexpr std::array<std::string_view, 3> kDragNames{
            "none", "text", "track"};

        [[nodiscard]] std::optional<KeyLayout> layoutFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kLayoutNames.size();
                 ++index)
            {
                if (kLayoutNames[index] == name)
                {
                    return static_cast<KeyLayout>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<Modal> modalFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kModalNames.size();
                 ++index)
            {
                if (kModalNames[index] == name)
                {
                    return static_cast<Modal>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<ui::DragHome> dragFromName(
            std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kDragNames.size();
                 ++index)
            {
                if (kDragNames[index] == name)
                {
                    return static_cast<ui::DragHome>(index);
                }
            }

            return std::nullopt;
        }

        // A whole-number pair, which is how every fraction here rides.
        nlohmann::json rationalSchema()
        {
            nlohmann::json schema;
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"num", "den"}; // GCOVR_EXCL_LINE
            schema["properties"]["num"]["type"] = "integer";
            schema["properties"]["den"]["type"] = "integer";

            // A denominator of zero is no fraction at all.
            schema["properties"]["den"]["minimum"] = 1;
            return schema;
        }

        nlohmann::json editorSchema()
        {
            nlohmann::json schema;
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "source",     "cursor",   "scroll", // GCOVR_EXCL_LINE
                "clipboard",  "layout",   "layoutOpen",
                "dragging",   "paused",   "menuOpen",
                "speed",      "speedOpen", "modal",
                "fileName",   "fileCursor", "notice",
                "scores"}; // GCOVR_EXCL_LINE

            auto &fields = schema["properties"];
            fields["source"]["type"] = "string";

            // No minimum on a caret, on purpose.
            // ui::kCaretAtEnd is the unsigned maximum.
            // The validator misreads that sentinel as negative.
            fields["cursor"]["type"] = "integer";
            fields["anchor"]["type"] = "integer";
            fields["scroll"]["type"] = "integer";
            fields["scroll"]["minimum"] = 0;
            fields["clipboard"]["type"] = "string";
            fields["layout"]["type"] = "string";
            fields["layoutOpen"]["type"] = "boolean";
            fields["dragging"]["type"] = "string";
            fields["paused"]["type"] = "boolean";
            fields["menuOpen"]["type"] = "boolean";

            // An index into kSpeeds, so the table bounds it.
            fields["speed"]["type"] = "integer";
            fields["speed"]["minimum"] = 0;
            fields["speed"]["maximum"] = kSpeeds.size() - 1;

            fields["speedOpen"]["type"] = "boolean";
            fields["modal"]["type"] = "string";
            fields["fileName"]["type"] = "string";
            fields["fileCursor"]["type"] = "integer";
            fields["notice"]["type"] = "string";
            fields["scores"]["type"] = "array";
            fields["scores"]["items"]["type"] = "string";
            return schema;
        }

        nlohmann::json playbackSchema()
        {
            nlohmann::json schema;
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "segments", "retimed", "played", // GCOVR_EXCL_LINE
                "counter",  "queued",  "pausedFrames",
                "voiceCount"}; // GCOVR_EXCL_LINE

            auto &fields = schema["properties"];
            fields["segments"]["type"] = "array";
            fields["segments"]["minItems"] = 1;

            auto &segment = fields["segments"]["items"];
            segment["type"] = "object";
            segment["additionalProperties"] = false;
            segment["required"] = {
                "startCycle", "framesPerCycle"}; // GCOVR_EXCL_LINE
            segment["properties"]["startCycle"] = rationalSchema();
            segment["properties"]["framesPerCycle"] = rationalSchema();

            fields["retimed"] = rationalSchema();
            fields["played"]["type"] = "integer";
            fields["played"]["minimum"] = 0;
            fields["counter"]["type"] = "integer";
            fields["counter"]["minimum"] = 0;
            fields["queued"]["type"] = "integer";
            fields["queued"]["minimum"] = 0;
            fields["pausedFrames"]["type"] = "integer";
            fields["pausedFrames"]["minimum"] = 0;
            fields["voiceCount"]["type"] = "integer";
            fields["voiceCount"]["minimum"] = 0;
            return schema;
        }

        nlohmann::json stateSchema()
        {
            nlohmann::json schema;
            schema["$schema"] =
                "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika music editor dump state";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "editor", "playback"}; // GCOVR_EXCL_LINE
            schema["properties"]["editor"] = editorSchema();
            schema["properties"]["playback"] = playbackSchema();
            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const nlohmann::json_schema::json_validator
                validator(stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        [[nodiscard]] nlohmann::json rationalToJson(
            const sequencer::Rational &value)
        {
            nlohmann::json encoded;
            encoded["num"] = value.numerator();
            encoded["den"] = value.denominator();
            return encoded;

            // gcov puts the returned value's unwind block here.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] sequencer::Rational rationalFromJson(
            const nlohmann::json &encoded)
        {
            // The schema already bound the denominator to one or more.
            // So the constructor's own zero refusal cannot fire here.
            return {
                encoded.at("num").get<std::int64_t>(),
                encoded.at("den").get<std::int64_t>()};
        }

        [[nodiscard]] nlohmann::json editorToJson(
            const EditorState &editor)
        {
            nlohmann::json encoded;
            encoded["source"] = editor.source;
            encoded["cursor"] =
                static_cast<std::uint64_t>(editor.cursor);

            // Absent means nothing is selected.
            // A member for it would be a place no selection ends.
            if (editor.anchor.has_value())
            {
                encoded["anchor"] =
                    static_cast<std::uint64_t>(*editor.anchor);
            }

            encoded["scroll"] =
                static_cast<std::uint64_t>(editor.scroll);
            encoded["clipboard"] = editor.clipboard;
            encoded["layout"] = std::string(
                kLayoutNames[static_cast<std::size_t>(editor.layout)
                             % kLayoutNames.size()]);
            encoded["layoutOpen"] = editor.layoutOpen;
            encoded["dragging"] = std::string(
                kDragNames[static_cast<std::size_t>(editor.dragging)
                           % kDragNames.size()]);
            encoded["paused"] = editor.paused;
            encoded["menuOpen"] = editor.menuOpen;
            encoded["speed"] =
                static_cast<std::uint64_t>(editor.speed);
            encoded["speedOpen"] = editor.speedOpen;
            encoded["modal"] = std::string(
                kModalNames[static_cast<std::size_t>(editor.modal)
                            % kModalNames.size()]);
            encoded["fileName"] = editor.fileName;
            encoded["fileCursor"] =
                static_cast<std::uint64_t>(editor.fileCursor);
            encoded["notice"] = editor.notice;
            encoded["scores"] = editor.scores;
            return encoded;

            // gcov puts the returned value's unwind block here.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] EditorState editorFromJson(
            const nlohmann::json &encoded)
        {
            EditorState editor;
            editor.source = encoded.at("source").get<std::string>();
            editor.cursor = static_cast<std::size_t>(
                encoded.at("cursor").get<std::uint64_t>());

            if (encoded.contains("anchor"))
            {
                editor.anchor = static_cast<std::size_t>(
                    encoded.at("anchor").get<std::uint64_t>());
            }
            else
            {
                editor.anchor = std::nullopt;
            }

            editor.scroll = static_cast<std::size_t>(
                encoded.at("scroll").get<std::uint64_t>());
            editor.clipboard =
                encoded.at("clipboard").get<std::string>();

            const auto layoutNamed =
                encoded.at("layout").get<std::string>();
            const auto layout = layoutFromName(layoutNamed);

            if (!layout.has_value())
            {
                throw StateDumpError(
                    "antwika::music_editor: dump names a keyboard "
                    "this build does not know: "
                    + layoutNamed);
            }

            editor.layout = *layout;
            editor.layoutOpen =
                encoded.at("layoutOpen").get<bool>();

            const auto dragNamed =
                encoded.at("dragging").get<std::string>();
            const auto dragging = dragFromName(dragNamed);

            if (!dragging.has_value())
            {
                throw StateDumpError(
                    "antwika::music_editor: dump names a drag home "
                    "this build does not know: "
                    + dragNamed);
            }

            editor.dragging = *dragging;
            editor.paused = encoded.at("paused").get<bool>();
            editor.menuOpen = encoded.at("menuOpen").get<bool>();
            editor.speed = static_cast<std::size_t>(
                encoded.at("speed").get<std::uint64_t>());
            editor.speedOpen = encoded.at("speedOpen").get<bool>();

            const auto modalNamed =
                encoded.at("modal").get<std::string>();
            const auto modal = modalFromName(modalNamed);

            if (!modal.has_value())
            {
                throw StateDumpError(
                    "antwika::music_editor: dump names a modal this "
                    "build does not know: "
                    + modalNamed);
            }

            editor.modal = *modal;
            editor.fileName =
                encoded.at("fileName").get<std::string>();
            editor.fileCursor = static_cast<std::size_t>(
                encoded.at("fileCursor").get<std::uint64_t>());
            editor.notice = encoded.at("notice").get<std::string>();
            editor.scores = encoded.at("scores")
                                .get<std::vector<std::string>>();
            return editor;
        }

        [[nodiscard]] nlohmann::json playbackToJson(
            const PlaybackMemory &playback)
        {
            nlohmann::json encoded;
            encoded["segments"] = nlohmann::json::array();

            // startFrame is deliberately not written.
            // Rebuilding the map recomputes every one identically.
            for (const auto &segment : playback.segments)
            {
                nlohmann::json each;
                each["startCycle"] =
                    rationalToJson(segment.startCycle);
                each["framesPerCycle"] =
                    rationalToJson(segment.framesPerCycle);
                encoded["segments"].push_back(std::move(each));
            }

            encoded["retimed"] = rationalToJson(playback.retimed);
            encoded["played"] = playback.played;
            encoded["counter"] = playback.counter;
            encoded["queued"] = playback.queued;
            encoded["pausedFrames"] = playback.pausedFrames;
            encoded["voiceCount"] =
                static_cast<std::uint64_t>(playback.voiceCount);
            return encoded;

            // gcov puts the returned value's unwind block here.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] PlaybackMemory playbackFromJson(
            const nlohmann::json &encoded)
        {
            PlaybackMemory playback;

            const auto &segments = encoded.at("segments");

            // The map's own zero is where every table begins.
            // A first segment elsewhere is a table with a hole.
            const auto firstStart = rationalFromJson(
                segments.at(0).at("startCycle"));

            if (firstStart != sequencer::Rational())
            {
                throw StateDumpError(
                    "antwika::music_editor: dump's first tempo "
                    "segment does not start at cycle zero");
            }

            // Rebuilt through the map itself.
            // That recomputes every startFrame on the way in.
            // And a broken table is refused before anything is kept.
            try
            {
                sequencer::TempoMap rebuilt(rationalFromJson(
                    segments.at(0).at("framesPerCycle")));

                for (std::size_t at = 1; at < segments.size(); ++at)
                {
                    rebuilt.addSegment(
                        rationalFromJson(
                            segments.at(at).at("startCycle")),
                        rationalFromJson(
                            segments.at(at).at("framesPerCycle")));
                }

                playback.segments = rebuilt.segments();
            }
            // The map's refusal is the sequencer's failure category.
            // What this format promises is StateDumpError.
            catch (const sequencer::SequencerError &refused)
            {
                throw StateDumpError(refused.what());
            }

            playback.retimed =
                rationalFromJson(encoded.at("retimed"));
            playback.played =
                encoded.at("played").get<time::Tick>();
            playback.counter =
                encoded.at("counter").get<std::uint64_t>();
            playback.queued =
                encoded.at("queued").get<FrameIndex>();
            playback.pausedFrames =
                encoded.at("pausedFrames").get<FrameIndex>();
            playback.voiceCount = static_cast<std::size_t>(
                encoded.at("voiceCount").get<std::uint64_t>());
            return playback;
        }
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        // No steps: version 1 is the first shape there has been.
        return antwika::replay::MigrationChain(
            {}, kStateDumpVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json editorDumpToJson(const EditorDump &dump)
    {
        nlohmann::json encoded;
        encoded["editor"] = editorToJson(dump.editor);
        encoded["playback"] = playbackToJson(dump.playback);
        return encoded;

        // gcov puts the returned value's unwind block here.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    EditorDump editorDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this format promises is StateDumpError.
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw StateDumpError(
                std::string(
                    "antwika::music_editor: dump state failed "
                    "schema validation: ")
                + failed.what());
        }

        EditorDump dump;
        dump.editor = editorFromJson(state.at("editor"));
        dump.playback = playbackFromJson(state.at("playback"));
        return dump;
    }

} // namespace antwika::music_editor
