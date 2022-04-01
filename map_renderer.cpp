#include "map_renderer.h"

#include <iostream>
#include <sstream>

namespace renderer {

detail::SphereProjector::SphereProjector(const transport_catalogue::TransportCatalogue& transport_catalogue,
                                         const RenderSettings& render_settings) {
    std::unordered_set<geo::Coordinates, CoordinatesHash> all_coordinates;

    for (const auto& bus : transport_catalogue.GetAllBuses()) {
        for (const auto stop_ptr : bus.stops) {
            all_coordinates.insert(stop_ptr->coordinates);
        }
    }

    SphereProjector sphere_projector{all_coordinates.begin(), all_coordinates.end(),
        render_settings.width, render_settings.height,
        render_settings.padding};

    std::swap(*this, sphere_projector);
}

MapRenderer::MapRenderer(const transport_catalogue::TransportCatalogue& transport_catalogue,
                         RenderSettings render_settings)
: transport_catalogue_(transport_catalogue), render_settings_(std::move(render_settings)) {
    detail::SphereProjector sphere_projector{transport_catalogue_, render_settings_};

    // create needed data
    Routes data;

    for (const auto& bus : transport_catalogue_.GetAllBuses()) {
        if (bus.stops.size() == 0) {
            continue;
        }

        std::vector<Location> coordinates_on_map;

        for (const auto& stop : bus.stops) {
            coordinates_on_map.push_back({stop->name, sphere_projector(stop->coordinates)});
        }

        if (!bus.is_circular) {
            auto before_last_index = static_cast<int>(coordinates_on_map.size()) - 2;

            for (int i = before_last_index; i >= 0; --i) {
                coordinates_on_map.push_back(coordinates_on_map.at(i));
            }
        }

        data[bus.name] = std::move(coordinates_on_map);
    }

    // save it
    data_ = std::move(data);
}

void MapRenderer::AddLines(svg::Document& document) const {
    const auto colors_in_palette = render_settings_.color_palette.size();

    int bus_counter = 0;
    for (const auto& [bus_name, route] : data_) {
        if (route.size() == 0) {
            continue;
        }

        svg::Polyline polyline;
        for (const auto& [stop_name, stop_point] : route) {
            polyline.AddPoint(stop_point);
        }

        int index_of_color_in_pallette = bus_counter++ % colors_in_palette;
        polyline.SetStrokeColor(render_settings_.color_palette.at(index_of_color_in_pallette))
            .SetFillColor(svg::NoneColor)
            .SetStrokeWidth(render_settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        document.Add(polyline);
    }
}

void MapRenderer::AddBusNames(svg::Document& document) const {
    const auto colors_in_palette = render_settings_.color_palette.size();

    int bus_counter = 0;

    for (const auto& [bus_name, route] : data_) {
        if (route.size() == 0) {
            continue;
        }

        const auto first_bus_stop = route.at(0);

        svg::Text text;

        int index_of_color_in_pallette = bus_counter++ % colors_in_palette;
        text.SetFillColor(render_settings_.color_palette.at(index_of_color_in_pallette))
            .SetPosition(first_bus_stop.point)
            .SetOffset(render_settings_.bus_label_offset)
            .SetFontSize(render_settings_.bus_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetFontWeight("bold"s)
            .SetData(bus_name);

        svg::Text text_underlayer = text;
        text_underlayer.SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        document.Add(text_underlayer);
        document.Add(text);

        const bool is_roundtrip = transport_catalogue_.GetBus(bus_name)->is_circular;
        const auto last_stop = (route.begin() + route.size() / 2)->point;

        if (!is_roundtrip && !(first_bus_stop.point == last_stop)) {
            document.Add(text_underlayer.SetPosition(last_stop));
            document.Add(text.SetPosition(last_stop));
        }
    }
}

std::map<std::string, svg::Point> GetUniqueStops(const Routes& data) {
    std::map<std::string, svg::Point> unique_stops;

    for (const auto& [bus_name, route] : data) {
        for (const auto& [stop_name, point] : route) {
            unique_stops[stop_name] = point;
        }
    }

    return unique_stops;
}

void MapRenderer::AddStopSymbols(svg::Document& document) const {
    svg::Circle circle;
    circle.SetRadius(render_settings_.stop_radius).SetFillColor("white"s);

    for (const auto& [name, point] : GetUniqueStops(data_)) {
        document.Add(circle.SetCenter(point));
    }
}

void MapRenderer::AddStopNames(svg::Document& document) const {
    svg::Text text;
    text.SetOffset(render_settings_.stop_label_offset)
        .SetFontSize(render_settings_.stop_label_font_size)
        .SetFontFamily("Verdana"s);

    auto text_underlayer = text;
    text_underlayer.SetFillColor(render_settings_.underlayer_color)
        .SetStrokeColor(render_settings_.underlayer_color)
        .SetStrokeWidth(render_settings_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    text.SetFillColor("black"s);

    for (const auto& [name, point] : GetUniqueStops(data_)) {
        document.Add(text_underlayer.SetPosition(point).SetData(name));
        document.Add(text.SetPosition(point).SetData(name));
    }
}

std::unique_ptr<svg::Document> MapRenderer::RenderMap() const {
    auto doc_ptr = std::make_unique<svg::Document>();

    AddLines(*doc_ptr);
    AddBusNames(*doc_ptr);
    AddStopSymbols(*doc_ptr);
    AddStopNames(*doc_ptr);

    return doc_ptr;
}

std::string MapRenderer::GetMapAsString() const {
    std::unique_ptr<svg::Document> svg_document_ptr = RenderMap();

    std::stringstream output_stream;
    svg_document_ptr->Render(output_stream);

    return output_stream.str();
}

svg::Point detail::SphereProjector::operator()(geo::Coordinates coords) const {
    return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
}

size_t CoordinatesHash::operator()(const geo::Coordinates& coordinates) const {
    size_t longitude_hashed = double_hasher_(coordinates.lng);
    size_t lattitude_hashed = double_hasher_(coordinates.lat);

    // 37 is a random prime number
    return 37 * longitude_hashed + lattitude_hashed;
}

std::unordered_set<geo::Coordinates, CoordinatesHash> GetAllPossibleCoordinates(
                                                                                const std::deque<Bus>& buses) {
    std::unordered_set<geo::Coordinates, CoordinatesHash> output;
    for (const auto& bus : buses) {
        for (const auto stop_ptr : bus.stops) {
            output.insert(stop_ptr->coordinates);
        }
    }

    return output;
}

}  // namespace renderer
