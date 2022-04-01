#pragma once

#include <optional>
#include <sstream>
#include <unordered_set>

#include "domain.h"
#include "json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

using namespace std::string_view_literals;

namespace transport_catalogue {

namespace request_handler {

class RequestHandler {
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const transport_catalogue::TransportCatalogue& db, const renderer::MapRenderer& renderer,
                   const router::TransportRouter& router);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<transport_catalogue::BusStatistics> GetBusStat(const std::string_view& bus_name) const;

    json::Node GetResponseToStatRequest(std::string_view type, int id, std::optional<std::string_view> name = {},
                                        std::optional<RouteInfo> route_info = {}) const;

    std::unique_ptr<svg::Document> RenderMap() const;

    const std::set<BusPtr, BusPointerComparator>* GetBusesByStop(const std::string_view& stop_name) const;

private:
    json::Node GetResponseToStopRequeset(std::string_view name, int id) const;

    json::Node GetResponseToBusRequest(std::string_view name, int id) const;

    json::Node GetResponseToMapRequest(int id) const;

    json::Node GetResponseToRouteRequest(int id, RouteInfo route_info) const;

private:
    const transport_catalogue::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    const router::TransportRouter& transport_router_;
};

}  // namespace request_handler

}  // namespace transport_catalogue
