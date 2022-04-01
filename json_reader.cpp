#include "json_reader.h"

#include "request_handler.h"

using namespace std::string_view_literals;

namespace transport_catalogue {

using namespace json_reader;

void JsonReader::ReadJson(std::istream& input_stream) {
    json::Document parsed_json_document = json::Load(input_stream);

    base_requests_ = parsed_json_document.GetRoot().AsDict().at("base_requests"s).AsArray();

    stat_requests_ = parsed_json_document.GetRoot().AsDict().at("stat_requests"s).AsArray();

    if (parsed_json_document.GetRoot().AsDict().count("render_settings"s)) {
        render_settings_ = parsed_json_document.GetRoot().AsDict().at("render_settings"s).AsDict();
    }

    if (parsed_json_document.GetRoot().AsDict().count("routing_settings"s)) {
        routing_settings_ = parsed_json_document.GetRoot().AsDict().at("routing_settings"s).AsDict();
    }
}

svg::Color ColorFromNodeArray(const json::Node& node) {
    assert(node.IsArray());

    auto color_values = node.AsArray();

    assert(color_values.size() >= 3 && color_values.size() < 5);

    int red = color_values.at(0).AsInt();
    int green = color_values.at(1).AsInt();
    int blue = color_values.at(2).AsInt();

    if (color_values.size() == 4) {
        return svg::Rgba{static_cast<uint8_t>(red), static_cast<uint8_t>(green), static_cast<uint8_t>(blue),
                         color_values.at(3).AsDouble()};
    }

    return svg::Rgb(red, green, blue);
}

svg::Color ColorFromNode(const json::Node& node) {
    if (node.IsString()) {
        return node.AsString();

    } else if (node.IsArray()) {
        return ColorFromNodeArray(node);
    }

    assert(false && "color parsing problem");
}

transport_catalogue::TransportCatalogue json_reader::ReadTransportCatalogue(
    const json::Array& base_requests_json) {
    TransportCatalogueDesctiption description;

    for (const auto& base_request : base_requests_json) {
        assert(base_request.IsDict());

        if (base_request.At<std::string>("type"s) == "Bus"s) {
            BusBaseRequest& bus = description.buses.emplace_back();

            bus.name = base_request.At<std::string>("name"s);

            for (const auto& node_that_holds_stop_name : base_request.AsDict().at("stops"s).AsArray()) {
                bus.stop_names.push_back(node_that_holds_stop_name.AsString());
            }

            bus.is_roundtrip = base_request.AsDict().at("is_roundtrip"s).AsBool();

        } else if (base_request.At<std::string>("type"s) == "Stop"s) {
            StopBaseRequest& stop = description.stops.emplace_back();

            stop.name = base_request.At<std::string>("name"s);

            stop.latitude = base_request.At<double>("latitude"s);

            stop.longitude = base_request.At<double>("longitude"s);

            for (const auto& [stop_name, distance] : base_request.AsDict().at("road_distances"s).AsDict()) {
                stop.road_distances.push_back({stop_name, distance.AsInt()});
            }

        } else {
            assert(false && "Invalid base request type");
        }
    }

    transport_catalogue::TransportCatalogue output;

    for (const auto& [name, latitude, longitude, road_distances] : description.stops) {
        output.AddStop(name, {latitude, longitude});
    }

    for (const auto& [name, stops, is_roundtrip] : description.buses) {
        output.AddBus(name, stops, is_roundtrip);
    }

    for (const auto& [from_name, latitude, longitude, road_distances] : description.stops) {
        for (const auto& [destination_name, distance] : road_distances) {
            output.AddDistancesBetweenStops(output.GetStop(from_name), output.GetStop(destination_name), distance);
        }
    }

    return output;
}

renderer::RenderSettings json_reader::ReadRenderSettings(const json::Dict& render_settings_json) {
    renderer::RenderSettings settings;

    settings.width = render_settings_json.at("width"s).AsDouble();
    settings.height = render_settings_json.at("height"s).AsDouble();

    settings.padding = render_settings_json.at("padding"s).AsDouble();

    settings.line_width = render_settings_json.at("line_width"s).AsDouble();

    settings.stop_radius = render_settings_json.at("stop_radius"s).AsDouble();

    settings.bus_label_font_size = render_settings_json.at("bus_label_font_size"s).AsInt();

    const auto bus_label_offsets = render_settings_json.at("bus_label_offset"s).AsArray();
    settings.bus_label_offset = {bus_label_offsets.at(0).AsDouble(), bus_label_offsets.at(1).AsDouble()};

    settings.stop_label_font_size = render_settings_json.at("stop_label_font_size"s).AsInt();

    const auto stop_label_offsets = render_settings_json.at("stop_label_offset"s).AsArray();
    settings.stop_label_offset = {stop_label_offsets.at(0).AsDouble(), stop_label_offsets.at(1).AsDouble()};

    settings.underlayer_color = ColorFromNode(render_settings_json.at("underlayer_color"s));

    settings.underlayer_width = render_settings_json.at("underlayer_width"s).AsDouble();

    for (const json::Node& color : render_settings_json.at("color_palette"s).AsArray()) {
        settings.color_palette.push_back(ColorFromNode(color));
    }

    return settings;
}

json::Array json_reader::HandleRequests(const json::Array& requests_json,
                                        const request_handler::RequestHandler& handler) {
    json::Array output;

    for (const json::Node& request : requests_json) {
        std::string_view type = request.AsDict().at("type"s).AsString();

        std::optional<std::string_view> name;
        if (type != "Map"sv && type != "Route"sv) {
            name = request.AsDict().at("name"s).AsString();
        }

        std::optional<RouteInfo> route_info;
        if (type == "Route"sv) {
            RouteInfo info;
            info.to = request.AsDict().at("to"s).AsString();
            info.from = request.AsDict().at("from"s).AsString();
            route_info = info;
        }

        int id = json::GetIntValue(request, "id"s);

        output.push_back(handler.GetResponseToStatRequest(type, id, name, route_info));
    }

    return output;
}

}  // namespace transport_catalogue
