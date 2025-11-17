// a_star.cpp (updated: respect oneway & drivable ways; nearest-node helper)

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <cmath>
#include <limits>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <string>

struct Node {
    double lat, lon;
};
struct Edge {
    int64_t to;
    double weight;
};

std::unordered_map<int64_t, Node> nodes;
std::unordered_map<int64_t, std::vector<Edge>> adj;

constexpr double PI_CONST = 3.14159265358979323846;
inline double deg2rad(double deg) { return deg * PI_CONST / 180.0; }

double haversine(double lat1, double lon1, double lat2, double lon2) {
    // Returns distance in meters
    const double R = 6371000.0; // mean Earth radius in meters
    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(deg2rad(lat1)) * std::cos(deg2rad(lat2)) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

// Helper: find nearest node id for a lat/lon (linear search - slow for full map, but fine for testing)
int64_t findNearestNode(double lat, double lon) {
    double bestDist = std::numeric_limits<double>::infinity();
    int64_t bestId = 0;
    for (const auto &p : nodes) {
        double d = haversine(lat, lon, p.second.lat, p.second.lon);
        if (d < bestDist) {
            bestDist = d;
            bestId = p.first;
        }
    }
    return bestId;
}

void loadKarachiMap(const std::string& filename) {
    struct MapHandler : public osmium::handler::Handler {
        // set of highway tags that are appropriate for motor vehicle routing
        const std::unordered_set<std::string> drivables = {
            "motorway","trunk","primary","secondary","tertiary",
            "unclassified","residential","service","living_street",
            "motorway_link","primary_link","secondary_link","tertiary_link"
        };

        // disallow these (pedestrian/cycle) types explicitly
        const std::unordered_set<std::string> nondrivable = {
            "footway","path","cycleway","steps","pedestrian","track","bridleway","corridor"
        };

        void node(const osmium::Node& node) {
            if (node.location().valid()) {
                nodes[node.id()] = {node.location().lat(), node.location().lon()};
            }
        }

        void way(const osmium::Way& way) {
            const char* highway_tag = way.tags()["highway"];
            if (!highway_tag) return; // not a highway/road-type way

            std::string hw = highway_tag;
            if (nondrivable.count(hw)) return; // skip pedestrian / cycle / steps etc.

            // allow ways that are in drivables set; if not present, skip to be conservative
            if (!drivables.count(hw)) {
                // there are some ambiguous 'road' ways; to be conservative, skip unknown kinds
                return;
            }

            // check simple access restrictions
            const char* access_tag = way.tags()["access"];
            const char* motor_tag = way.tags()["motor_vehicle"];
            if ((access_tag && std::string(access_tag) == "no") ||
                (motor_tag && std::string(motor_tag) == "no")) {
                return; // not allowed for motor vehicles
            }

            // determine one-way behavior
            bool oneway = false;
            bool oneway_reverse = false;
            const char* oneway_tag = way.tags()["oneway"];
            const char* junction_tag = way.tags()["junction"];
            if (junction_tag && std::string(junction_tag) == "roundabout") {
                oneway = true;
            }
            if (oneway_tag) {
                std::string ow(oneway_tag);
                if (ow == "yes" || ow == "true" || ow == "1") oneway = true;
                else if (ow == "-1") oneway_reverse = true;
            }

            const osmium::WayNodeList& wnl = way.nodes();
            // add edges according to the directionality indicated by tags
            for (auto it = wnl.begin(); std::next(it) != wnl.end(); ++it) {
                int64_t id1 = it->ref();
                int64_t id2 = std::next(it)->ref();
                if (!nodes.count(id1) || !nodes.count(id2)) continue; // skip if coordinates unknown

                double d = haversine(nodes[id1].lat, nodes[id1].lon,
                                     nodes[id2].lat, nodes[id2].lon);

                if (oneway_reverse) {
                    // edge only from id2 -> id1
                    adj[id2].push_back({id1, d});
                } else if (oneway) {
                    // edge only from id1 -> id2 (way node order)
                    adj[id1].push_back({id2, d});
                } else {
                    // bidirectional (normal two-way street)
                    adj[id1].push_back({id2, d});
                    adj[id2].push_back({id1, d});
                }
            }
        }
    };

    try {
        osmium::io::Reader reader(filename);
        MapHandler handler;
        osmium::apply(reader, handler);
        reader.close();
        std::cout << "Map loaded successfully! Nodes: " << nodes.size()
                  << "  Adjacencies (non-empty keys): " << adj.size() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error reading Karachi map: " << e.what() << "\n";
    }
}

std::vector<int64_t> astar(int64_t start, int64_t goal) {
    std::unordered_map<int64_t, double> gScore;
    std::unordered_map<int64_t, double> fScore;
    std::unordered_map<int64_t, int64_t> parent;

    gScore[start] = 0.0;
    fScore[start] = haversine(nodes[start].lat, nodes[start].lon,
                              nodes[goal].lat, nodes[goal].lon);

    auto cmp = [](const std::pair<int64_t, double>& a, const std::pair<int64_t, double>& b) {
        return a.second > b.second;
    };
    std::priority_queue<std::pair<int64_t, double>,
                       std::vector<std::pair<int64_t, double>>,
                       decltype(cmp)> openSet(cmp);

    openSet.push({start, fScore[start]});

    int nodes_explored = 0;

    while (!openSet.empty()) {
        auto current_pair = openSet.top();
        openSet.pop();
        int64_t current = current_pair.first;
        double current_fscore_in_queue = current_pair.second;

        if (fScore.count(current) && current_fscore_in_queue > fScore[current] + 1e-9) {
            continue; // stale entry
        }

        nodes_explored++;

        if (current == goal) {
            std::vector<int64_t> path;
            for (int64_t at = goal; at != start; at = parent[at]) {
                path.push_back(at);
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            std::cout << "Path found! Nodes explored: " << nodes_explored << "\n";
            return path;
        }

        if (!adj.count(current)) continue;

        for (const auto& edge : adj[current]) {
            double tentative_gScore = gScore[current] + edge.weight;

            if (!gScore.count(edge.to) || tentative_gScore < gScore[edge.to]) {
                parent[edge.to] = current;
                gScore[edge.to] = tentative_gScore;
                fScore[edge.to] = tentative_gScore +
                    haversine(nodes[edge.to].lat, nodes[edge.to].lon,
                              nodes[goal].lat, nodes[goal].lon);

                openSet.push({edge.to, fScore[edge.to]});
            }
        }
    }

    std::cout << "No path found after exploring " << nodes_explored << " nodes.\n";
    return {};
}

void aStar() {
    const std::string map_file = "res/data/karachi.osm.pbf";
    loadKarachiMap(map_file);

    std::cout << "Do you want to enter (1) node IDs or (2) coordinates? Enter 1 or 2: ";
    int mode = 1;
    std::cin >> mode;

    int64_t start = 0, goal = 0;

    if (mode == 1) {
        std::cout << "Enter start node ID: ";
        std::cin >> start;
        std::cout << "Enter goal node ID: ";
        std::cin >> goal;
    } else {
        double slat, slon, glat, glon;
        std::cout << "Enter start latitude: ";
        std::cin >> slat;
        std::cout << "Enter start longitude: ";
        std::cin >> slon;
        std::cout << "Enter goal latitude: ";
        std::cin >> glat;
        std::cout << "Enter goal longitude: ";
        std::cin >> glon;

        start = findNearestNode(slat, slon);
        goal  = findNearestNode(glat, glon);

        std::cout << "Nearest start node: " << start
                  << "  (lat: " << nodes[start].lat << " lon: " << nodes[start].lon << ")\n";
        std::cout << "Nearest goal node: " << goal
                  << "  (lat: " << nodes[goal].lat << " lon: " << nodes[goal].lon << ")\n";
    }

    if (!nodes.count(start) || !nodes.count(goal)) {
        std::cerr << "Invalid node IDs (not found in loaded OSM nodes).\n";
        return;
    }

    // Generate output file name
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::stringstream filename;
    filename << "res/data/path_output_"
             << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".txt";

    std::ofstream outfile(filename.str());
    if (!outfile) {
        std::cerr << "Failed to create output file.\n";
        return;
    }

    double straight_distance = haversine(nodes[start].lat, nodes[start].lon,
                                         nodes[goal].lat, nodes[goal].lon);
    std::cout << "Straight-line distance: " << straight_distance / 1000.0 << " km\n";

    std::cout << "Calculating shortest path...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<int64_t> path = astar(start, goal);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    outfile << "Start Node ID: " << start << "\n";
    outfile << "Goal Node ID: " << goal << "\n";
    outfile << "Straight-line distance: " << straight_distance / 1000.0 << " km\n";
    outfile << "Calculation time: " << duration.count() << " ms\n";
    outfile << "------------------------------------\n";

    if (path.empty()) {
        outfile << "No path found between given nodes.\n";
        std::cout << "No path found between given nodes.\n";
    } else {
        double total = 0;
        outfile << "Shortest path:\n";
        for (size_t i = 0; i < path.size(); ++i) {
            outfile << path[i];
            if (i + 1 < path.size()) {
                double d = haversine(nodes[path[i]].lat, nodes[path[i]].lon,
                                     nodes[path[i + 1]].lat, nodes[path[i + 1]].lon);
                total += d;
                outfile << " -> ";
            }
        }
        outfile << "\nTotal distance: " << total / 1000.0 << " km\n";
        outfile << "Path length: " << path.size() << " nodes\n";
        outfile << "Efficiency ratio: " << (straight_distance > 0 ? (total / straight_distance) : 0.0) << " (ideal: ~1.0)\n";

        std::cout << "Path saved successfully to: " << filename.str() << "\n";
        std::cout << "Total distance: " << total / 1000.0 << " km\n";
        std::cout << "Efficiency ratio: " << (straight_distance > 0 ? (total / straight_distance) : 0.0) << " (ideal: ~1.0)\n";

        if (total / straight_distance > 1.3) {
            std::cout << "WARNING: Path is significantly longer than straight-line distance!\n";
            std::cout << "This may indicate missing direct road connections or a routing restriction in the OSM data.\n";
        }
    }

    outfile.close();
}