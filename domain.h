#pragma once

#include <string>
#include <vector>

#include "geo.h"

struct Bus;

struct Stop;

using StopPtr = const Stop*;

using BusPtr = const Bus*;

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<StopPtr> stops;
    bool is_circular = false;
};

struct BusPointerComparator {
    bool operator()(const Bus* left, const Bus* right) const { return left->name < right->name; }
};

struct DistanceBetweenStopsHash {
    size_t operator()(const std::pair<StopPtr, StopPtr>& pair_of_stops) const {
        size_t from_hashed = poiner_hasher_(pair_of_stops.first);
        size_t to_hashed = poiner_hasher_(pair_of_stops.first);

        // 37 is a random prime number
        return 37 * from_hashed + to_hashed;
    }

private:
    std::hash<const void*> poiner_hasher_;
};

struct RoutingSettings {
    int wait_time;
    double velocity;
};

struct RouteInfo {
    std::string_view from;
    std::string_view to;
};
