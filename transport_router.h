#pragma once

#include <memory>
#include <variant>

#include "domain.h"
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport_catalogue {

namespace router {

using Minutes = double;

struct VertexIds {
    graph::VertexId in{};
    graph::VertexId out{};
};

constexpr Minutes kZeroWaitTime{};

class TransportRouter {
public:
    struct BusRideInfo {
        std::string_view bus_name;
        int span_count{};
        Minutes time{};
    };

    struct WaitInfo {
        std::string_view stop_name;
        Minutes time{};
    };

    using RouteItem = std::variant<std::monostate, WaitInfo, BusRideInfo>;

public:
    TransportRouter(const transport_catalogue::TransportCatalogue& catalogue, const RoutingSettings& settings);

    std::optional<std::pair<Minutes, std::vector<RouteItem>>> GetRouteInfo(StopPtr stop_from,
                                                                           StopPtr stop_to) const;

private:
    void CreateEdges(const TransportCatalogue& catalogue) {
        CreateWaitEdges(catalogue.GetAllStops());
        CreateBusEdges(catalogue);
    }

    size_t CreateVertexes(const std::deque<Stop>& stops);

    void CreateWaitEdges(const std::deque<Stop>& stops);

    void CreateBusEdges(const TransportCatalogue& catalogue) {
        for (const Bus& bus : catalogue.GetAllBuses()) {
            ConnectStations(bus.stops.begin(), bus.stops.end(), bus.name);

            if (!bus.is_circular) {
                ConnectStations(bus.stops.rbegin(), bus.stops.rend(), bus.name);
            }
        }
    }

    template <typename It>
    void ConnectStations(It begin, It end, std::string_view bus_name) {
        for (auto from_it = begin; from_it != std::prev(end); ++from_it) {
            Minutes weight{};
            int span_count{};

            for (auto to_it = std::next(from_it); to_it != end; ++to_it) {
                std::string_view departure_name = (*from_it)->name;

                graph::VertexId departure = vertexes_.at(departure_name).out;

                std::string_view arrival_name = (*to_it)->name;

                graph::VertexId arrival = vertexes_.at(arrival_name).in;

                weight += CalculateTimeBetweenStations(*prev(to_it), *(to_it));
                ++span_count;

                auto bus_edge_id = graph_.AddEdge({departure, arrival, weight});

                bus_edges_[bus_edge_id] = {bus_name, span_count, weight};
            }
        }
    }

    Minutes CalculateTimeBetweenStations(StopPtr from, StopPtr to) const;

    graph::VertexId GenereateNewVertexId() { return vertexes_counter_++; }

private:
    const transport_catalogue::TransportCatalogue& catalogue_;

    RoutingSettings settings_;

    graph::DirectedWeightedGraph<Minutes> graph_;

    std::unique_ptr<graph::Router<Minutes>> router_;

    std::unordered_map<std::string_view, VertexIds> vertexes_;

    graph::VertexId vertexes_counter_ = 0;

    // use this containers to remember which are wait edges
    std::unordered_map<graph::EdgeId, WaitInfo> wait_edges_;

    // use this containers to remember which are ride edges
    std::unordered_map<graph::EdgeId, BusRideInfo> bus_edges_;
};

}  // namespace router

}  // namespace transport_catalogue
