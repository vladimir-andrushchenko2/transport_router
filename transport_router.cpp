#include "transport_router.h"

namespace transport_catalogue {

using namespace router;

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& catalogue,
                                 const RoutingSettings& settings)
    : catalogue_(catalogue), settings_(settings) {
    graph_ = graph::DirectedWeightedGraph<Minutes>(CreateVertexes(catalogue.GetAllStops()));
    CreateEdges(catalogue);
    router_ = std::make_unique<graph::Router<Minutes>>(graph_);
}

std::optional<std::pair<Minutes, std::vector<TransportRouter::RouteItem>>> TransportRouter::GetRouteInfo(
    StopPtr stop_from, StopPtr stop_to) const {
    graph::VertexId from_vertex = vertexes_.at(stop_from->name).in;

    graph::VertexId to_vertex = vertexes_.at(stop_to->name).in;

    std::optional<graph::Router<Minutes>::RouteInfo> route_info = router_->BuildRoute(from_vertex, to_vertex);

    if (!route_info) {
        return {};
    }

    std::pair<Minutes, std::vector<RouteItem>> output;
    output.first = route_info->weight;

    auto& items = output.second;

    for (const auto& edge_id : route_info->edges) {
        if (bus_edges_.count(edge_id) > 0) {
            items.push_back(bus_edges_.at(edge_id));

        } else if (wait_edges_.count(edge_id) > 0) {
            items.push_back(wait_edges_.at(edge_id));

        } else {
            assert(false);
        }
    }

    return {std::move(output)};
}

size_t TransportRouter::CreateVertexes(const std::deque<Stop>& stops) {
    for (const Stop& stop : stops) {
        vertexes_[stop.name].in = GenereateNewVertexId();
        vertexes_[stop.name].out = GenereateNewVertexId();
    }

    return vertexes_counter_;
}

void TransportRouter::CreateWaitEdges(const std::deque<Stop>& stops) {
    for (const Stop& stop : stops) {
        wait_edges_.insert({graph_.AddEdge({vertexes_.at(stop.name).in, vertexes_.at(stop.name).out,
                                            static_cast<Minutes>(settings_.wait_time)}),
                            {stop.name, static_cast<Minutes>(settings_.wait_time)}});
    }
}

Minutes TransportRouter::CalculateTimeBetweenStations(StopPtr from, StopPtr to) const {
    return 60.0 * catalogue_.GetDistanceBetweenStops(from, to) / (1000.0 * settings_.velocity);
}

}  // namespace transport_catalogue
