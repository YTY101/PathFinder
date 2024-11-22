#include <nlohmann/json.hpp>
#include <unordered_set>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <vector>
#include <string>

#include "findpath.h"
#include "server.h"
#include "read_data.h"

using json = nlohmann::json;


// A* 算法实现
json aStarSearch(const std::string& start_id, const std::string& end_id) {
    std::priority_queue<std::pair<double, std::string>, std::vector<std::pair<double, std::string>>, std::greater<>> openSet;
    std::unordered_map<std::string, std::string> cameFrom;
    std::unordered_map<std::string, double> gScore; // 从起点到当前节点的代价
    std::unordered_map<std::string, double> fScore; // 从起点到终点的估计总代价
    std::unordered_map<std::string, bool> openSetMap; // 用于追踪节点是否在开放列表中

    for (const auto& pair : nodes) {
        gScore[pair.first] = std::numeric_limits<double>::infinity();
        fScore[pair.first] = std::numeric_limits<double>::infinity();
    }

    gScore[start_id] = 0.0;
    fScore[start_id] = calculateDistance(nodes[start_id], nodes[end_id]);
    openSet.emplace(fScore[start_id], start_id);
    openSetMap[start_id] = true; // 将起点标记为在开放列表中

    while (!openSet.empty()) {
        std::string current = openSet.top().second;
        openSet.pop();
         openSetMap.erase(current); // 从哈希表中移除该节点]

        if (current == end_id) {
            // 重新构建路径
            json path = json::array();
            for (std::string node = end_id; !node.empty(); node = cameFrom[node]) {
                const Node& n = nodes[node];
                path.push_back({{"id", n.id}, {"lat", n.lat}, {"lng", n.lon}});
            }
            std::reverse(path.begin(), path.end()); // 反转路径
            return path;
        }

        for (const Neighbor& neighbor : neighbors[current]) {
            double tentative_gScore = gScore[current] + neighbor.distance;

            if (tentative_gScore < gScore[neighbor.node_id]) {
                cameFrom[neighbor.node_id] = current;
                gScore[neighbor.node_id] = tentative_gScore;
                fScore[neighbor.node_id] = gScore[neighbor.node_id] + calculateDistance(nodes[neighbor.node_id], nodes[end_id]);

                // 如果不在开放列表中，加进去
                if (!openSetMap[neighbor.node_id]) {
                    openSet.emplace(fScore[neighbor.node_id], neighbor.node_id);
                    openSetMap[neighbor.node_id] = true; // 更新哈希表
                }
            }
        }
    }

    // 如果没有找到路径，返回空 JSON
    return json::array();
}
