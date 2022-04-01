#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <variant>

#include "geo.h"
#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

using namespace std::string_literals;

namespace transport_catalogue {

namespace json_reader {

struct BusBaseRequest {
    std::string name;
    std::vector<std::string> stop_names;
    bool is_roundtrip = false;
};

struct StopBaseRequest {
    std::string name;
    double latitude = 0.0;
    double longitude = 0.0;
    std::vector<std::pair<std::string, int>> road_distances;
};

struct TransportCatalogueDesctiption {
    std::deque<BusBaseRequest> buses;
    std::deque<StopBaseRequest> stops;
};

transport_catalogue::TransportCatalogue ReadTransportCatalogue(const json::Array& base_requests_json);

renderer::RenderSettings ReadRenderSettings(const json::Dict& render_settings_json);

json::Array HandleRequests(const json::Array& requests_json, const request_handler::RequestHandler& handler);

inline RoutingSettings BuildRoutingSettings(const json::Node& node) {
    RoutingSettings routing_settings;

    routing_settings.velocity = node.At<double>("bus_velocity"s);
    routing_settings.wait_time = node.At<int>("bus_wait_time"s);

    return routing_settings;
}

class JsonReader {
public:
    void ReadJson(std::istream& input_stream);

    const json::Array& GetBaseRequests() const { return base_requests_; }

    const json::Array& GetStatRequests() const { return stat_requests_; }

    const json::Dict& GetRenderSettings() const { return render_settings_; }

    const json::Dict& GetRoutingSettings() const { return routing_settings_; }

private:
    json::Dict routing_settings_;
    json::Dict render_settings_;
    json::Array base_requests_;
    json::Array stat_requests_;
};

}  // namespace json_reader

}  // namespace transport_catalogue
