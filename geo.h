#pragma once

namespace geo {

struct Coordinates {
    double lat; // Широта
    double lng; // Долгота
    
    bool operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
};

double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo
