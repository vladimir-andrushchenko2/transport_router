#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace renderer {

struct RenderSettings {
    double width{};
    double height{};
    double padding{};
    double line_width{};
    double stop_radius{};
    uint64_t bus_label_font_size{};
    svg::Point bus_label_offset;
    uint64_t stop_label_font_size{};
    svg::Point stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width{};
    std::vector<svg::Color> color_palette;
};

struct CoordinatesHash {
    size_t operator()(const geo::Coordinates& coordinates) const;

private:
    std::hash<double> double_hasher_;
};

std::unordered_set<geo::Coordinates, CoordinatesHash> GetAllPossibleCoordinates(
                                                                                const std::deque<Bus>& buses);

namespace detail {

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) { return std::abs(value) < EPSILON; }

class SphereProjector {
public:
    SphereProjector(const transport_catalogue::TransportCatalogue& transport_catalogue,
                    const RenderSettings& render_settings);

    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                    double max_height, double padding);

    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

}  // namespace detail

struct Location {
    std::string name;
    svg::Point point;
};

using Routes = std::map<std::string, std::vector<Location>>;

class MapRenderer {
public:
    MapRenderer(const transport_catalogue::TransportCatalogue& transport_catalogue,
                RenderSettings render_settings);

    std::string GetMapAsString() const;

    std::unique_ptr<svg::Document> RenderMap() const;

private:
    void AddLines(svg::Document& document) const;

    void AddBusNames(svg::Document& document) const;

    void AddStopSymbols(svg::Document& document) const;

    void AddStopNames(svg::Document& document) const;

private:
    const transport_catalogue::TransportCatalogue& transport_catalogue_;
    RenderSettings render_settings_;
    Routes data_;
};

}  // namespace renderer

template <typename PointInputIt>
renderer::detail::SphereProjector::SphereProjector(PointInputIt points_begin,
                                                   PointInputIt points_end, double max_width,
                                                   double max_height, double padding)
: padding_(padding) {
    if (points_begin == points_end) {
        return;
    }

    const auto [left_it, right_it] = std::minmax_element(
                                                         points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    const auto [bottom_it, top_it] = std::minmax_element(
                                                         points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        zoom_coeff_ = *height_zoom;
    }
}
