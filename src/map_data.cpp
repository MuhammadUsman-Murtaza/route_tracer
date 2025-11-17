// Map_Data.cpp (modified to include node lat/lon output)

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <osmium/io/any_input.hpp>
#include <osmium/handler.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/way.hpp>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <sstream>

struct Road {
    std::string name;
    std::string type;
    std::vector<std::vector<osmium::object_id_type>> segments; // Each "Way" is one segment
};

class MyHandler : public osmium::handler::Handler {
public:
    std::map<std::pair<std::string, std::string>, Road> mergedRoads;
    std::unordered_map<osmium::object_id_type, std::pair<double, double>> node_coords;

    void node(const osmium::Node& node) {
        if (node.location().valid()) {
            node_coords[node.id()] = { node.location().lat(), node.location().lon() };
        }
    }

    void way(const osmium::Way& way) {
        const char* highway = way.tags()["highway"];
        const char* name = way.tags()["name"];

        static const std::unordered_set<std::string> major_roads = {
            "motorway", "trunk", "primary", "secondary", "tertiary"
        };

        if (highway && name && major_roads.count(highway)) {
            std::pair<std::string, std::string> key(name, highway);

            std::vector<osmium::object_id_type> nodes;
            for (const auto& node_ref : way.nodes()) {
                nodes.push_back(node_ref.ref());
            }

            auto& road = mergedRoads[key];
            if (road.name.empty()) {
                road.name = name;
                road.type = highway;
            }
            road.segments.push_back(nodes);
        }
    }

    void printMergedData(std::ostream& out) const {
        for (const auto& entry : mergedRoads) {
            const auto& road = entry.second;

            out << "Road: " << road.name
                << " | Type: " << road.type
                << " | Segments: " << road.segments.size()
                << "\n";

            for (size_t i = 0; i < road.segments.size(); ++i) {
                const auto& seg = road.segments[i];
                if (!seg.empty()) {
                    out << "  Segment " << (i + 1)
                        << " → Nodes: " << seg.front()
                        << " ... " << seg.back()
                        << " (" << seg.size() << " nodes)\n";

                    auto print_coord = [&](osmium::object_id_type nid) {
                        auto it = node_coords.find(nid);
                        if (it != node_coords.end()) {
                            out << "     Node " << nid << " [lat: " << std::fixed << std::setprecision(7)
                                << it->second.first << ", lon: " << it->second.second << "]\n";
                        } else {
                            out << "     Node " << nid << " [lat/lon: unknown]\n";
                        }
                    };

                    // show first node coords
                    print_coord(seg.front());
                    // if more than 1 node, show last node coords
                    if (seg.size() > 1) {
                        print_coord(seg.back());
                    }

                    // (optional) show up to first 3 intermediate nodes' coords to help debugging
                    size_t show_count = std::min<size_t>(3, seg.size());
                    if (seg.size() > 2) {
                        out << "     Sample intermediate nodes:\n";
                        for (size_t k = 1; k <= show_count && k + 1 < seg.size(); ++k) {
                            osmium::object_id_type nid = seg[k];
                            auto it = node_coords.find(nid);
                            if (it != node_coords.end()) {
                                out << "       " << nid << " [lat: " << std::fixed << std::setprecision(7)
                                    << it->second.first << ", lon: " << it->second.second << "]\n";
                            } else {
                                out << "       " << nid << " [lat/lon: unknown]\n";
                            }
                        }
                    }
                }
            }
            out << "------------------------------------\n";
        }
    }
};

void parseMap() {
    const std::string input_file = "res/data/karachi.osm.pbf";

    try {
        osmium::io::Reader reader(input_file);
        MyHandler handler;

        osmium::apply(reader, handler);
        reader.close();

        // Generate txt file
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::stringstream filename;
        filename << "res/data/output_"
                 << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S")
                 << ".txt";

        std::ofstream outfile(filename.str());
        if (!outfile) {
            std::cerr << "Failed to create output file.\n";
            return;
        }

        handler.printMergedData(outfile);
        outfile.close();

        std::cout << "Map data successfully written to: " << filename.str() << "\n";
        std::cout << "Map data successfully merged and displayed.\n";

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
    }
}