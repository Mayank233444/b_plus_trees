#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "httplib.h"
#include "json.hpp"
#include "BPlusTree.hpp"

using json = nlohmann::json;

namespace {
BPlusTree tree(4, 3);
void sendJson(httplib::Response& response, const json& body, int status = 200) { response.status = status; response.set_content(body.dump(), "application/json"); }
ll parseWholeNumber(const std::string& value) {
    std::size_t used = 0;
    const ll parsed = std::stoll(value, &used);
    if (used != value.size()) throw std::invalid_argument("trailing characters");
    return parsed;
}
bool readKey(const httplib::Request& request, httplib::Response& response, ll& key) {
    if (!request.has_param("key")) { sendJson(response, {{"error", "A key is required."}}, 400); return false; }
    try {
        key = parseWholeNumber(request.get_param_value("key")); return true;
    } catch (const std::exception&) { sendJson(response, {{"error", "Key must be a whole number."}}, 400); return false; }
}
json operationResponse(const std::string& operation, ll key, const SnapshotManager& manager) {
    json result = {{"operation", operation}, {"value", key}, {"snapshots", json::array()}};
    for (const Snapshot& snapshot : manager.snapshots) result["snapshots"].push_back({{"message", snapshot.message}, {"state", snapshot.state}, {"highlightNodeId", snapshot.highlightNodeId}});
    return result;
}
}

int main() {
    httplib::Server server;
    server.set_mount_point("/", "frontend");
    server.Post("/api/config", [](const httplib::Request& request, httplib::Response& response) {
        try {
            if (!request.has_param("order") || !request.has_param("leafCapacity")) throw std::invalid_argument("missing");
            const ll order = parseWholeNumber(request.get_param_value("order")); const ll leafCapacity = parseWholeNumber(request.get_param_value("leafCapacity"));
            if (order < 3 || leafCapacity < 2 || order > 12 || leafCapacity > 12) throw std::out_of_range("range");
            tree.reset(order, leafCapacity); sendJson(response, {{"message", "Tree reset with the selected capacities."}, {"state", tree.toJson()}});
        } catch (const std::exception&) { sendJson(response, {{"error", "Internal order must be 3–12 and leaf capacity must be 2–12."}}, 400); }
    });
    server.Post("/api/insert", [](const httplib::Request& request, httplib::Response& response) {
        ll key; if (!readKey(request, response, key)) return; SnapshotManager snapshots; const bool changed = tree.insert(key, &snapshots); json result = operationResponse("insert", key, snapshots); result["changed"] = changed; sendJson(response, result);
    });
    server.Post("/api/delete", [](const httplib::Request& request, httplib::Response& response) {
        ll key; if (!readKey(request, response, key)) return; SnapshotManager snapshots; const bool changed = tree.erase(key, &snapshots); json result = operationResponse("delete", key, snapshots); result["changed"] = changed; sendJson(response, result);
    });
    server.Post("/api/search", [](const httplib::Request& request, httplib::Response& response) {
        ll key; if (!readKey(request, response, key)) return; SnapshotManager snapshots; const bool found = tree.search(key, &snapshots); json result = operationResponse("search", key, snapshots); result["found"] = found; sendJson(response, result);
    });
    server.Get("/api/state", [](const httplib::Request&, httplib::Response& response) { sendJson(response, tree.toJson()); });
    std::cout << "B+ Tree visualizer: http://localhost:8080/visualizer.html" << std::endl;
    server.listen("0.0.0.0", 8080);
}
