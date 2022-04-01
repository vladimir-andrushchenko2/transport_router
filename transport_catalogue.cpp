#include "transport_catalogue.h"

#include <unordered_set>

#include "geo.h"

using TransportCatalogue = transport_catalogue::TransportCatalogue;

int StopsOnRoute(const TransportCatalogue& transport_catalogue, const std::string& bus_name);

int UniqueStopsOnRoute(const TransportCatalogue& transport_catalogue, const std::string& bus_name);

double CalculateDistanceOfRouteUsingCoordinates(const TransportCatalogue& transport_catalogue,
                                                const std::string& bus_name);

int CalculateDistanceOfRouteUsingActualMeasurements(const TransportCatalogue& transport_catalogue,
                                                    const std::string& bus_name);

namespace transport_catalogue {

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string>& stop_names, bool is_circular) {
    // create new bus
    Bus new_bus;

    new_bus.name = std::move(name);

    new_bus.is_circular = is_circular;

    // add stations
    new_bus.stops.reserve(stop_names.size());

    for (const auto& stop : stop_names) {
        assert(stops_.count(stop) > 0);
        new_bus.stops.push_back(stops_.at(stop));
    }

    // add new bus to transport catalogue
    buses_storage_.push_back(std::move(new_bus));

    buses_[buses_storage_.back().name] = &buses_storage_.back();

    // register that Bus goes through the Stops
    for (const auto& stop : stop_names) {
        const Stop* stored_stop = GetStop(stop);
        stop_to_buses_[stored_stop].insert(&buses_storage_.back());
    }
}

void TransportCatalogue::AddStop(std::string name, geo::Coordinates coordinates) {
    // create stop
    Stop new_stop;
    new_stop.name = std::move(name);
    new_stop.coordinates = coordinates;

    // add stop to transport catalogue
    stop_storage_.push_back(std::move(new_stop));

    stops_[stop_storage_.back().name] = &stop_storage_.back();
}

void TransportCatalogue::AddDistancesBetweenStops(const Stop* from, const Stop* to, int distance) {
    distances_between_stops_[std::pair(from, to)] = distance;
}

int TransportCatalogue::GetDistanceBetweenStops(StopPtr from, const Stop* to) const {
    auto iter = distances_between_stops_.find({from, to});

    if (iter == distances_between_stops_.end()) {
        iter = distances_between_stops_.find({to, from});

        if (iter == distances_between_stops_.end()) {
            return -1;
        }
    }

    return iter->second;
}

bool TransportCatalogue::ContainsDistanceBetweenStops(const Stop* from, const Stop* to) const {
    return distances_between_stops_.count(std::pair(from, to)) > 0 ? true : false;
}

BusPtr TransportCatalogue::GetBus(std::string_view bus_name) const { return buses_.at(bus_name); }

size_t TransportCatalogue::CountBus(std::string_view bus_name) const { return buses_.count(bus_name); }

StopPtr TransportCatalogue::GetStop(std::string_view stop_name) const { return stops_.at(stop_name); }

size_t TransportCatalogue::CountStop(std::string_view stop_name) const { return stops_.count(stop_name); }

std::optional<const std::set<BusPtr, BusPointerComparator>*> TransportCatalogue::GetBusesThatPassStop(
    StopPtr stop) const {
    if (stop_to_buses_.count(stop) == 0) {
        return {};
    }

    return {&stop_to_buses_.at(stop)};
}

const std::deque<Bus>& TransportCatalogue::GetAllBuses() const { return buses_storage_; }

BusStatistics TransportCatalogue::GetBusStatistics(BusPtr bus) const {
    return {StopsOnRoute(bus), UniqueStopsOnRoute(bus), CalculateRouteDistanceUsingActualMeasurements(bus),
            CalculateRouteDistanceUsingCoordinates(bus)};
}

int TransportCatalogue::StopsOnRoute(BusPtr bus) const {
    int stations_listed_in_route = static_cast<int>(bus->stops.size());

    if (bus->is_circular) {
        return static_cast<int>(stations_listed_in_route);
    }

    // from Stop0 to StopN-1 to StopN to StopN-1 to Stop0, so to end and back and -1
    // because StopN is visited once
    return stations_listed_in_route * 2 - 1;
}

int TransportCatalogue::UniqueStopsOnRoute(BusPtr bus) const {
    std::unordered_set<std::string> unique_stops;

    for (const auto stop : bus->stops) {
        unique_stops.insert(stop->name);
    }

    return static_cast<int>(unique_stops.size());
}

double TransportCatalogue::CalculateRouteDistanceUsingCoordinates(BusPtr bus) const {
    const auto bus_route = bus->stops;

    double distance = 0.0;

    for (size_t i = 0; i < bus_route.size() - 1; ++i) {
        distance += ComputeDistance(bus_route[i]->coordinates, bus_route[i + 1]->coordinates);
    }

    if (!bus->is_circular) {
        distance *= 2;
    }

    return distance;
}

double TransportCatalogue::CalculateRouteDistanceUsingActualMeasurements(BusPtr bus) const {
    const auto bus_route = bus->stops;
    int bus_route_length = static_cast<int>(bus_route.size());

    double total_distance = 0;

    for (int i = 0; i < bus_route_length - 1; ++i) {
        auto from = GetStop(bus_route[i]->name);
        auto to = GetStop(bus_route[i + 1]->name);

        int distance_between_stops = GetDistanceBetweenStops(from, to);

        assert(distance_between_stops != -1);

        total_distance += distance_between_stops;
    }

    // go back if route is not circular
    if (!bus->is_circular) {
        for (int i = bus_route_length - 1; i > 0; --i) {
            auto from = GetStop(bus_route[i]->name);
            auto to = GetStop(bus_route[i - 1]->name);

            total_distance += GetDistanceBetweenStops(from, to);
        }
    }

    return total_distance;
}

}  // namespace transport_catalogue
