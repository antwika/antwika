#include "antwika/atlas_editor/StateDump.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <string>

#include <nlohmann/json-schema.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        // The names a dump document holds, one per tool.
        // Persisted, so they may not change once written.
        constexpr std::array<std::string_view, kToolCount> kToolNames{
            "paint", "erase", "fill", "pick", "select"};

        [[nodiscard]] std::string_view toolName(const Tool tool) noexcept
        {
            return kToolNames[
                static_cast<std::size_t>(tool) % kToolCount];
        }

        [[nodiscard]] std::optional<Tool> toolFromName(
            const std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kToolCount; ++index)
            {
                if (kToolNames[index] == name)
                {
                    return static_cast<Tool>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] nlohmann::json imageSchema()
        {
            nlohmann::json image;
            image["type"] = "object";
            image["additionalProperties"] = false;
            image["required"] = {
                "width", "height", "fingerprint"}; // GCOVR_EXCL_LINE
            image["properties"]["width"]["type"] = "integer";
            image["properties"]["width"]["minimum"] = 1;
            image["properties"]["height"]["type"] = "integer";
            image["properties"]["height"]["minimum"] = 1;

            // No minimum, deliberately: a fingerprint fills 64 bits.
            // A signed bound would read the top half as negative.
            image["properties"]["fingerprint"]["type"] = "integer";
            return image;
        }

        [[nodiscard]] nlohmann::json pixelSchema()
        {
            nlohmann::json pixel;
            pixel["type"] = "object";
            pixel["additionalProperties"] = false;
            pixel["required"] = {"x", "y"}; // GCOVR_EXCL_LINE
            pixel["properties"]["x"]["type"] = "integer";
            pixel["properties"]["y"]["type"] = "integer";
            return pixel;
        }

        [[nodiscard]] nlohmann::json stateSchema()
        {
            nlohmann::json schema;
            schema["$schema"] =
                "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika atlas editor dump state";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "sheet",
                "view",
                "tool",
                "paint",
                "showGrid",
                "showGuides",
                "counters"}; // GCOVR_EXCL_LINE

            schema["properties"]["sheet"] = imageSchema();
            schema["properties"]["sheet"]["required"].push_back(
                "revision");
            schema["properties"]["sheet"]["properties"]["revision"]
                  ["type"] = "integer";
            schema["properties"]["sheet"]["properties"]["revision"]
                  ["minimum"] = 0;

            schema["properties"]["clipboard"] = imageSchema();

            auto &view = schema["properties"]["view"];
            view["type"] = "object";
            view["additionalProperties"] = false;
            view["required"] = {
                "panX", "panY", "zoom"}; // GCOVR_EXCL_LINE
            view["properties"]["panX"]["type"] = "integer";
            view["properties"]["panY"]["type"] = "integer";
            view["properties"]["zoom"]["type"] = "integer";
            view["properties"]["zoom"]["minimum"] = 0;

            schema["properties"]["tool"]["type"] = "string";

            auto &paint = schema["properties"]["paint"];
            paint["type"] = "object";
            paint["additionalProperties"] = false;
            paint["required"] = {"r", "g", "b", "a"}; // GCOVR_EXCL_LINE
            for (const auto *channel : {"r", "g", "b", "a"})
            {
                paint["properties"][channel]["type"] = "integer";
                paint["properties"][channel]["minimum"] = 0;
                paint["properties"][channel]["maximum"] = 255;
            }

            schema["properties"]["swatch"]["type"] = "integer";
            schema["properties"]["swatch"]["minimum"] = 0;
            schema["properties"]["showGrid"]["type"] = "boolean";
            schema["properties"]["showGuides"]["type"] = "boolean";
            schema["properties"]["under"] = pixelSchema();

            auto &marked = schema["properties"]["marked"];
            marked["type"] = "object";
            marked["additionalProperties"] = false;
            marked["required"] = {
                "x", "y", "width", "height"}; // GCOVR_EXCL_LINE
            marked["properties"]["x"]["type"] = "integer";
            marked["properties"]["y"]["type"] = "integer";
            marked["properties"]["width"]["type"] = "integer";
            marked["properties"]["width"]["minimum"] = 1;
            marked["properties"]["height"]["type"] = "integer";
            marked["properties"]["height"]["minimum"] = 1;

            auto &gesture = schema["properties"]["gesture"];
            gesture["type"] = "object";
            gesture["additionalProperties"] = false;
            gesture["required"] = {
                "carrying", "from", "to"}; // GCOVR_EXCL_LINE
            gesture["properties"]["carrying"]["type"] = "boolean";
            gesture["properties"]["from"] = pixelSchema();
            gesture["properties"]["to"] = pixelSchema();

            auto &counters = schema["properties"]["counters"];
            counters["type"] = "object";
            counters["additionalProperties"] = false;
            counters["required"] = {
                "changes",
                "stepped",
                "written",
                "read",
                "savedRevision"}; // GCOVR_EXCL_LINE
            for (const auto *count :
                 {"changes", "stepped", "written", "read",
                  "savedRevision"})
            {
                counters["properties"][count]["type"] = "integer";
                counters["properties"][count]["minimum"] = 0;
            }

            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const nlohmann::json_schema::json_validator validator(
                stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        [[nodiscard]] nlohmann::json imageToJson(
            const DumpedImage &image)
        {
            nlohmann::json encoded;
            encoded["width"] = image.size.width;
            encoded["height"] = image.size.height;
            encoded["fingerprint"] = image.fingerprint;
            return encoded;
        }

        [[nodiscard]] DumpedImage imageFromJson(
            const nlohmann::json &image)
        {
            return DumpedImage{
                .size =
                    {.width = image.at("width").get<std::uint32_t>(),
                     .height = image.at("height").get<std::uint32_t>()},
                .fingerprint =
                    image.at("fingerprint").get<std::uint64_t>()};
        }

        [[nodiscard]] nlohmann::json pixelToJson(const Pixel pixel)
        {
            nlohmann::json encoded;
            encoded["x"] = pixel.x;
            encoded["y"] = pixel.y;
            return encoded;
        }

        [[nodiscard]] Pixel pixelFromJson(const nlohmann::json &pixel)
        {
            return Pixel{
                .x = pixel.at("x").get<std::int32_t>(),
                .y = pixel.at("y").get<std::int32_t>()};
        }
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        return antwika::replay::MigrationChain(
            {}, kStateDumpVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json stateDumpToJson(const EditorStateDump &dump)
    {
        nlohmann::json encoded;

        encoded["sheet"] = imageToJson(dump.sheet);
        encoded["sheet"]["revision"] = dump.sheetRevision;

        // Absent means nothing was in hand.
        // A member for it would be a shape for no clipboard.
        if (dump.clipboard.has_value())
        {
            encoded["clipboard"] = imageToJson(*dump.clipboard);
        }

        encoded["view"]["panX"] = dump.view.pan.x;
        encoded["view"]["panY"] = dump.view.pan.y;
        encoded["view"]["zoom"] = dump.view.zoom;

        encoded["tool"] = std::string(toolName(dump.tool));

        encoded["paint"]["r"] = dump.paint.red;
        encoded["paint"]["g"] = dump.paint.green;
        encoded["paint"]["b"] = dump.paint.blue;
        encoded["paint"]["a"] = dump.paint.alpha;

        if (dump.swatch.has_value())
        {
            encoded["swatch"] = *dump.swatch;
        }

        encoded["showGrid"] = dump.showGrid;
        encoded["showGuides"] = dump.showGuides;

        if (dump.under.has_value())
        {
            encoded["under"] = pixelToJson(*dump.under);
        }

        if (dump.marked.has_value())
        {
            encoded["marked"]["x"] = dump.marked->origin.x;
            encoded["marked"]["y"] = dump.marked->origin.y;
            encoded["marked"]["width"] = dump.marked->size.width;
            encoded["marked"]["height"] = dump.marked->size.height;
        }

        if (dump.gesture.has_value())
        {
            encoded["gesture"]["carrying"] = dump.gesture->carrying;
            encoded["gesture"]["from"] =
                pixelToJson(dump.gesture->from);
            encoded["gesture"]["to"] = pixelToJson(dump.gesture->to);
        }

        encoded["counters"]["changes"] = dump.changes;
        encoded["counters"]["stepped"] = dump.stepped;
        encoded["counters"]["written"] = dump.written;
        encoded["counters"]["read"] = dump.read;
        encoded["counters"]["savedRevision"] = dump.savedRevision;

        return encoded;

        // gcov puts the returned value's unwind block here.
        // game's StateDump.cpp encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    EditorStateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this format promises is AtlasEditorError.
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw AtlasEditorError(
                std::string(
                    "atlas_editor: dump state failed schema "
                    "validation: ")
                + failed.what());
        }

        EditorStateDump dump;

        dump.sheet = imageFromJson(state.at("sheet"));
        dump.sheetRevision =
            state.at("sheet").at("revision").get<std::uint64_t>();

        if (state.contains("clipboard"))
        {
            dump.clipboard = imageFromJson(state.at("clipboard"));
        }

        const auto &view = state.at("view");
        dump.view.pan.x = view.at("panX").get<std::int32_t>();
        dump.view.pan.y = view.at("panY").get<std::int32_t>();
        dump.view.zoom = view.at("zoom").get<std::size_t>();

        if (dump.view.zoom >= kZoomScales.size())
        {
            throw AtlasEditorError(
                "atlas_editor: dump names zoom level "
                + std::to_string(dump.view.zoom)
                + ", and the table ends at "
                + std::to_string(kZoomScales.size() - 1));
        }

        const auto named = state.at("tool").get<std::string>();
        const auto tool = toolFromName(named);

        if (!tool.has_value())
        {
            throw AtlasEditorError(
                "atlas_editor: dump names a tool this build does "
                "not know: "
                + named);
        }

        dump.tool = *tool;

        const auto &paint = state.at("paint");
        dump.paint.red = paint.at("r").get<std::uint8_t>();
        dump.paint.green = paint.at("g").get<std::uint8_t>();
        dump.paint.blue = paint.at("b").get<std::uint8_t>();
        dump.paint.alpha = paint.at("a").get<std::uint8_t>();

        if (state.contains("swatch"))
        {
            dump.swatch = state.at("swatch").get<std::size_t>();
        }

        dump.showGrid = state.at("showGrid").get<bool>();
        dump.showGuides = state.at("showGuides").get<bool>();

        if (state.contains("under"))
        {
            dump.under = pixelFromJson(state.at("under"));
        }

        if (state.contains("marked"))
        {
            const auto &marked = state.at("marked");
            dump.marked = Selection{
                .origin =
                    {.x = marked.at("x").get<std::int32_t>(),
                     .y = marked.at("y").get<std::int32_t>()},
                .size =
                    {.width =
                         marked.at("width").get<std::uint32_t>(),
                     .height =
                         marked.at("height").get<std::uint32_t>()}};
        }

        if (state.contains("gesture"))
        {
            const auto &gesture = state.at("gesture");
            dump.gesture = Gesture{
                .carrying = gesture.at("carrying").get<bool>(),
                .from = pixelFromJson(gesture.at("from")),
                .to = pixelFromJson(gesture.at("to"))};
        }

        const auto &counters = state.at("counters");
        dump.changes =
            counters.at("changes").get<std::uint64_t>();
        dump.stepped =
            counters.at("stepped").get<std::uint64_t>();
        dump.written =
            counters.at("written").get<std::uint32_t>();
        dump.read = counters.at("read").get<std::uint32_t>();
        dump.savedRevision =
            counters.at("savedRevision").get<std::uint64_t>();

        return dump;
    }

} // namespace antwika::atlas_editor
