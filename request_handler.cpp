#include "request_handler.h"

#include "json_builder.h"

namespace transport_catalogue {

using namespace request_handler;

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& db,
                               const renderer::MapRenderer& renderer, const router::TransportRouter& router)
    : db_(db), renderer_(renderer), transport_router_(router) {}

std::optional<transport_catalogue::BusStatistics> RequestHandler::GetBusStat(
    const std::string_view& bus_name) const {
    if (db_.CountBus(bus_name) > 0) {
        auto bus = db_.GetBus(bus_name);
        return {db_.GetBusStatistics(bus)};
    }

    return {};
}

json::Node RequestHandler::GetResponseToMapRequest(int id) const {
    return json::Builder{}
        .StartDict()
        .Key("request_id"s)
        .Value(id)
        .Key("map"s)
        .Value(renderer_.GetMapAsString())
        .EndDict()
        .Build();
}

json::Node RequestHandler::GetResponseToStatRequest(std::string_view type, int id,
                                                    std::optional<std::string_view> name,
                                                    std::optional<RouteInfo> route_info) const {
    if (type == "Stop"s) {
        return GetResponseToStopRequeset(name.value(), id);
    } else if (type == "Bus"s) {
        return GetResponseToBusRequest(name.value(), id);
    } else if (type == "Map"s) {
        return GetResponseToMapRequest(id);
    } else if (type == "Route"s) {
        return GetResponseToRouteRequest(id, route_info.value());
    }

    throw std::logic_error("unsupported type"s);
}

json::Node RequestHandler::GetResponseToRouteRequest(int id, RouteInfo route_info) const {
    using namespace router;

    StopPtr from = db_.GetStop(route_info.from);
    StopPtr to = db_.GetStop(route_info.to);

    std::optional<std::pair<Minutes, std::vector<TransportRouter::RouteItem>>> route =
        transport_router_.GetRouteInfo(from, to);

    if (!route) {
        return json::Builder{}
            .StartDict()
            .Key("request_id"s)
            .Value(id)
            .Key("error_message"s)
            .Value("not found"s)
            .EndDict()
            .Build();
    }

    json::Builder builder_node;

    builder_node.StartDict()
        .Key("request_id"s)
        .Value(id)
        .Key("total_time"s)
        .Value(route->first)
        .Key("items"s)
        .StartArray();

    for (const auto& route_item : route->second) {
        if (const auto wait_info_ptr = std::get_if<TransportRouter::WaitInfo>(&route_item)) {
            builder_node.StartDict()
                .Key("type"s)
                .Value("Wait"s)
                .Key("stop_name"s)
                .Value(std::string(wait_info_ptr->stop_name))
                .Key("time")
                .Value(wait_info_ptr->time)
                .EndDict();
        } else if (const auto ride_info_prt = std::get_if<TransportRouter::BusRideInfo>(&route_item)) {
            builder_node.StartDict()
                .Key("type"s)
                .Value("Bus"s)
                .Key("bus"s)
                .Value(std::string(ride_info_prt->bus_name))
                .Key("span_count"s)
                .Value(ride_info_prt->span_count)
                .Key("time"s)
                .Value(ride_info_prt->time)
                .EndDict();
        } else {
            assert(false);
        }
    }

    return builder_node.EndArray().EndDict().Build();
}

json::Node RequestHandler::GetResponseToStopRequeset(std::string_view name, int id) const {
    // if stop not found return error message
    if (db_.CountStop(name) == 0) {
        return json::Builder{}
            .StartDict()
            .Key("request_id"s)
            .Value(id)
            .Key("error_message"s)
            .Value("not found"s)
            .EndDict()
            .Build();
    }

    // turn found buses into nodes by emplacing into json::Array
    json::Array buses;

    auto buses_that_pass_stop = db_.GetBusesThatPassStop(db_.GetStop(name));

    // there can be stop that is not visited by any bus
    if (buses_that_pass_stop) {
        for (const auto bus_ptr : *buses_that_pass_stop.value()) {
            buses.emplace_back(bus_ptr->name);
        }
    }

    // put response to other responses
    return json::Builder{}.StartDict().Key("request_id"s).Value(id).Key("buses"s).Value(buses).EndDict().Build();
}

json::Node RequestHandler::GetResponseToBusRequest(std::string_view name, int id) const {
    auto is_statistics_found = GetBusStat(name);

    if (is_statistics_found) {
        auto statistics = is_statistics_found.value();

        return json::Builder{}
            .StartDict()
            .Key("curvature"s)
            .Value(statistics.route_distance_measured / statistics.route_distance_direct)
            .Key("route_length"s)
            .Value(statistics.route_distance_measured)
            .Key("stop_count"s)
            .Value(statistics.total_stops)
            .Key("unique_stop_count"s)
            .Value(statistics.unique_stops)
            .Key("request_id"s)
            .Value(id)
            .EndDict()
            .Build();
    }

    return json::Builder{}
        .StartDict()
        .Key("request_id"s)
        .Value(id)
        .Key("error_message"s)
        .Value("not found"s)
        .EndDict()
        .Build();
}

std::unique_ptr<svg::Document> RequestHandler::RenderMap() const { return renderer_.RenderMap(); }

const std::set<BusPtr, BusPointerComparator>* RequestHandler::GetBusesByStop(
    const std::string_view& stop_name) const {
    std::optional<const std::set<BusPtr, BusPointerComparator>*> output =
        db_.GetBusesThatPassStop(db_.GetStop(stop_name));

    if (output) {
        return *output;
    }

    std::stringstream error_msg;
    error_msg << stop_name << "doesn't exist"sv;
    throw std::logic_error(error_msg.str());
}

}  // namespace transport_catalogue
