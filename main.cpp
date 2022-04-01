#include <iostream>
#include <sstream>

#include "json.h"
#include "json_reader.h"
#include "request_handler.h"
#include "test_string.h"
#include "transport_catalogue.h"
#include "transport_router.h"

using namespace std::string_literals;
using namespace transport_catalogue;
using namespace json_reader;
using namespace request_handler;

std::istringstream test_input{test_string};

int main() {
    JsonReader json_reader;

    json_reader.ReadJson(test_input);

    TransportCatalogue transport_catalogue = json_reader::ReadTransportCatalogue(json_reader.GetBaseRequests());

    renderer::MapRenderer map_renderer(transport_catalogue,
                                       json_reader::ReadRenderSettings(json_reader.GetRenderSettings()));

    router::TransportRouter router(transport_catalogue, BuildRoutingSettings(json_reader.GetRoutingSettings()));

    RequestHandler request_handler{transport_catalogue, map_renderer, router};

    auto response = json_reader::HandleRequests(json_reader.GetStatRequests(), request_handler);

    json::Print(json::Document(response), std::cout);
}
