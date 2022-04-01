#pragma once

#include <cassert>
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain.h"
#include "geo.h"

using namespace std::string_literals;

namespace transport_catalogue {

struct BusStatistics {
    int total_stops{};
    int unique_stops{};
    double route_distance_measured{};
    double route_distance_direct{};
};

class TransportCatalogue {
public:
    void AddBus(std::string name, const std::vector<std::string>& stop_names, bool is_circular);

    void AddStop(std::string name, geo::Coordinates coordinates);

    // after all Stops have been added, initialize distances
    void AddDistancesBetweenStops(StopPtr from, StopPtr to, int distance);

    int GetDistanceBetweenStops(StopPtr from, StopPtr to) const;

    bool ContainsDistanceBetweenStops(StopPtr from, StopPtr to) const;

    BusPtr GetBus(std::string_view bus_name) const;

    size_t CountBus(std::string_view bus_name) const;

    StopPtr GetStop(std::string_view stop_name) const;

    size_t CountStop(std::string_view stop_name) const;

    std::optional<const std::set<BusPtr, BusPointerComparator>*> GetBusesThatPassStop(StopPtr stop) const;

    BusStatistics GetBusStatistics(BusPtr bus) const;

    const std::deque<Bus>& GetAllBuses() const;

    const std::deque<Stop>& GetAllStops() const { return stop_storage_; }

private:
    int StopsOnRoute(BusPtr bus) const;

    int UniqueStopsOnRoute(BusPtr bus) const;

    double CalculateRouteDistanceUsingCoordinates(BusPtr bus) const;

    double CalculateRouteDistanceUsingActualMeasurements(BusPtr bus) const;

private:
    std::deque<Bus> buses_storage_;
    std::deque<Stop> stop_storage_;
    std::unordered_map<std::string_view, StopPtr> stops_;
    std::unordered_map<std::string_view, BusPtr> buses_;

    std::map<StopPtr, std::set<BusPtr, BusPointerComparator>> stop_to_buses_;

    std::unordered_map<std::pair<StopPtr, StopPtr>, int, DistanceBetweenStopsHash> distances_between_stops_;
};

}  // namespace transport_catalogue
