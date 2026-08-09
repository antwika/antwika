#include "antwika/atlas_editor/StateDump.hpp"

#include <cstddef>
#include <exception>
#include <string>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/enums/NameTable.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"

namespace antwika::atlas_editor
{

    namespace
    {
        using antwika::replay::countShape;
        using antwika::replay::objectShape;
        using antwika::replay::wordShape;

        constexpr antwika::enums::NameTable<Tool> kTools{
            {"paint", "erase", "fill", "pick", "select", "line",
             "ellipse"}};

        [[nodiscard]] nlohmann::json imageSchema()
        {
            nlohmann::json image =
                objectShape({"width", "height", "fingerprint"});
            image["properties"]["width"]["type"] = "integer";
            image["properties"]["width"]["minimum"] = 1;
            image["properties"]["height"]["type"] = "integer";
            image["properties"]["height"]["minimum"] = 1;

            image["properties"]["fingerprint"]["type"] = "integer";
            return image;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json pixelSchema()
        {
            nlohmann::json pixel = objectShape({"x", "y"});
            pixel["properties"]["x"]["type"] = "integer";
            pixel["properties"]["y"]["type"] = "integer";
            return pixel;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] PreviewPane previewFromJson(
            const nlohmann::json &encoded)
        {
            PreviewPane pane;
            pane.open = encoded.at("open").get<bool>();
            pane.autoFocus = encoded.at("autoFocus").get<bool>();
            pane.ratio = encoded.at("ratio").get<std::uint32_t>();
            pane.view.pan.x = encoded.at("panX").get<std::int32_t>();
            pane.view.pan.y = encoded.at("panY").get<std::int32_t>();
            pane.view.zoom = encoded.at("zoom").get<std::size_t>();

            if (pane.view.zoom >= kZoomScales.size())
            {
                throw AtlasEditorError(
                    "atlas_editor: dump names preview zoom level "
                    + std::to_string(pane.view.zoom)
                    + ", and the table ends at "
                    + std::to_string(kZoomScales.size() - 1));
            }

            if (encoded.contains("focused"))
            {
                pane.focused =
                    encoded.at("focused").get<std::uint32_t>();
            }

            return pane;
        }

        [[nodiscard]] nlohmann::json stateSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "antwika atlas editor dump state",
                {"sheet",
                 "view",
                 "tool",
                 "paint",
                 "showGrid",
                 "showGuides",
                 "counters"});

            schema["properties"]["sheet"] = imageSchema();
            schema["properties"]["sheet"]["required"].push_back(
                "revision");
            schema["properties"]["sheet"]["properties"]["revision"] =
                countShape();

            schema["properties"]["clipboard"] = imageSchema();

            auto &view = schema["properties"]["view"];
            view = objectShape({"panX", "panY", "zoom"});
            view["properties"]["panX"]["type"] = "integer";
            view["properties"]["panY"]["type"] = "integer";
            view["properties"]["zoom"] = countShape();

            schema["properties"]["tool"] = wordShape();

            auto &paint = schema["properties"]["paint"];
            paint = objectShape({"r", "g", "b", "a"});
            for (const auto *channel : {"r", "g", "b", "a"})
            {
                paint["properties"][channel] =
                    antwika::replay::boundedCountShape(255);
            }

            auto &preview = schema["properties"]["preview"];
            preview = objectShape(
                {"open", "autoFocus", "ratio", "panX", "panY", "zoom"});
            preview["properties"]["open"]["type"] = "boolean";
            preview["properties"]["autoFocus"]["type"] = "boolean";
            preview["properties"]["ratio"] =
                antwika::replay::boundedCountShape(
                    antwika::ui::kWholeSplit);
            preview["properties"]["panX"]["type"] = "integer";
            preview["properties"]["panY"]["type"] = "integer";
            preview["properties"]["zoom"] = countShape();
            preview["properties"]["focused"] = countShape();

            schema["properties"]["swatch"] = countShape();
            schema["properties"]["showGrid"]["type"] = "boolean";
            schema["properties"]["showGuides"]["type"] = "boolean";
            schema["properties"]["showPivot"]["type"] = "boolean";
            schema["properties"]["showPointerBorder"]["type"] =
                "boolean";
            schema["properties"]["under"] = pixelSchema();

            auto &marked = schema["properties"]["marked"];
            marked = objectShape({"x", "y", "width", "height"});
            marked["properties"]["x"]["type"] = "integer";
            marked["properties"]["y"]["type"] = "integer";
            marked["properties"]["width"]["type"] = "integer";
            marked["properties"]["width"]["minimum"] = 1;
            marked["properties"]["height"]["type"] = "integer";
            marked["properties"]["height"]["minimum"] = 1;

            auto &gesture = schema["properties"]["gesture"];
            gesture = objectShape({"carrying", "from", "to"});
            gesture["properties"]["carrying"]["type"] = "boolean";
            gesture["properties"]["from"] = pixelSchema();
            gesture["properties"]["to"] = pixelSchema();

            auto &counters = schema["properties"]["counters"];
            counters = objectShape(
                {"changes",
                 "stepped",
                 "written",
                 "read",
                 "savedRevision"});
            for (const auto *count :
                 {"changes", "stepped", "written", "read",
                  "savedRevision"})
            {
                counters["properties"][count] = countShape();
            }

            return schema;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json imageToJson(
            const DumpedImage &image)
        {
            nlohmann::json encoded;
            encoded["width"] = image.size.width;
            encoded["height"] = image.size.height;
            encoded["fingerprint"] = image.fingerprint;
            return encoded;

        } // GCOVR_EXCL_LINE

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

        } // GCOVR_EXCL_LINE

        [[nodiscard]] Pixel pixelFromJson(const nlohmann::json &pixel)
        {
            return Pixel{
                .x = pixel.at("x").get<std::int32_t>(),
                .y = pixel.at("y").get<std::int32_t>()};
        }
    }

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

        if (dump.clipboard.has_value())
        {
            encoded["clipboard"] = imageToJson(*dump.clipboard);
        }

        encoded["view"]["panX"] = dump.view.pan.x;
        encoded["view"]["panY"] = dump.view.pan.y;
        encoded["view"]["zoom"] = dump.view.zoom;

        encoded["preview"]["open"] = dump.preview.open;
        encoded["preview"]["autoFocus"] = dump.preview.autoFocus;
        encoded["preview"]["ratio"] = dump.preview.ratio;
        encoded["preview"]["panX"] = dump.preview.view.pan.x;
        encoded["preview"]["panY"] = dump.preview.view.pan.y;
        encoded["preview"]["zoom"] = dump.preview.view.zoom;

        if (dump.preview.focused.has_value())
        {
            encoded["preview"]["focused"] = *dump.preview.focused;
        }

        encoded["tool"] = std::string(kTools.name(dump.tool));

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
        encoded["showPivot"] = dump.showPivot;
        encoded["showPointerBorder"] = dump.showPointerBorder;

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

    } // GCOVR_EXCL_LINE

    EditorStateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            antwika::replay::validatorFor<stateSchema>().validate(
                state);
        }
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

        if (state.contains("preview"))
        {
            dump.preview = previewFromJson(state.at("preview"));
        }

        const auto named = state.at("tool").get<std::string>();
        const auto tool = kTools.from(named);

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

        if (state.contains("showPivot"))
        {
            dump.showPivot = state.at("showPivot").get<bool>();
        }

        if (state.contains("showPointerBorder"))
        {
            dump.showPointerBorder =
                state.at("showPointerBorder").get<bool>();
        }

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

}
