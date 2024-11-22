#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <tinyxml2.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <cmath>
#include <utility>
#include "server.h"
#include "read_data.h"

#define CHUNK_SIZE 0.007 // 定义一个 CHUNK_SIZE，单位是经纬度的差值

using namespace tinyxml2;
using namespace std;

// 生成 CHUNK 的键值
string getChunkKey(double lat, double lon) {
    int lat_index = static_cast<int>(lat / CHUNK_SIZE);
    int lon_index = static_cast<int>(lon / CHUNK_SIZE);
    return to_string(lat_index) + "_" + to_string(lon_index);
}

std::pair<string, int> getTargetChunk(string chunk_id, int depth, unordered_map<string, bool>& visited){
    visited[chunk_id] = true; // 标记当前 Chunk 为已访问
    int idX = 0, idY = 0;
    // 使用 stringstream 解析字符串
    std::stringstream ss(chunk_id);
    std::string token;

    // 拆分 idX
    std::getline(ss, token, '_');
    idX = std::stoi(token); // 转换为整数

    // 拆分 idY
    std::getline(ss, token, '_');
    idY = std::stoi(token); // 转换为整数

    if(chunk_map[chunk_id].nodes.size() > 0){
        return {chunk_id, depth};
    }else{
        // 检查上下左右的 Chunk
        vector<pair<string, int>> results; // 上下左右
        vector<pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // 上下左右
        for (const auto& dir : directions) {
            int new_idX = idX + dir.first;
            int new_idY = idY + dir.second;
            string new_chunk_id = to_string(new_idX) + "_" + to_string(new_idY);
            if (visited.find(new_chunk_id) == visited.end()) {
                visited[new_chunk_id] = true;
                auto result = getTargetChunk(new_chunk_id, depth + 1, visited);
                if (!result.first.empty()) {
                    results.push_back(result);
                }
            }
        }
        // 选择深度最小的 Chunk
        int min_depth = INT_MAX;
        string min_id = "";
        for(const auto& result : results){
            if(result.second < min_depth){
                min_depth = result.second;
                min_id = result.first;
            }
        }
        return std::make_pair(min_id, min_depth);
    }
}


// 检查 Node 是否在指定的 Chunk 范围内
bool isNodeInChunk(const Node& node, double min_lat, double max_lat, double min_lon, double max_lon) {
    return (node.lat >= min_lat && node.lat <= max_lat && node.lon >= min_lon && node.lon <= max_lon);
}

// 计算两个节点之间的距离（简单的欧几里得距离）
double calculateDistance(const Node& node1, const Node& node2) {
    return sqrt(pow(node1.lat - node2.lat, 2) + pow(node1.lon - node2.lon, 2));
}
std::unordered_map<std::string, std::vector<Neighbor>> neighbors;

// 解析 OSM 文件的函数
void parseOSM(const string& filename, unordered_map<string, Node>& nodes, unordered_map<string, Way>& ways, unordered_map<string, Chunk>& chunk_map) {
    XMLDocument doc;
    XMLError eResult = doc.LoadFile(filename.c_str());
    if (eResult != XML_SUCCESS) {
        cout << "Error loading OSM file!" << endl;
        cout << eResult << endl;
        return;
    }

    // 解析 Node
    XMLNode* pRoot = doc.FirstChildElement("osm");
    XMLElement* pNodeElement = pRoot->FirstChildElement("node");
    while (pNodeElement) {
        Node node;
        node.id = pNodeElement->Attribute("id"); // 读取为string
        node.lat = pNodeElement->DoubleAttribute("lat");
        node.lon = pNodeElement->DoubleAttribute("lon");

        // 解析 Node 的 Tag
        XMLElement* pTagElement = pNodeElement->FirstChildElement("tag");
        while (pTagElement) {
            string key = pTagElement->Attribute("k");
            string value = pTagElement->Attribute("v");
            node.tags.push_back(make_pair(key, value));
            pTagElement = pTagElement->NextSiblingElement("tag");
        }

        nodes[node.id] = node; // 将 Node 存储到 unordered_map 中

        // 将 Node 存储到对应的 Chunk 中
        string chunk_key = getChunkKey(node.lat, node.lon);
        chunk_map[chunk_key].nodes.push_back(node);

        pNodeElement = pNodeElement->NextSiblingElement("node");
    }

    // 解析 Way
    XMLElement* pWayElement = pRoot->FirstChildElement("way");
    while (pWayElement) {
        Way way;
        way.id = pWayElement->Attribute("id"); // 读取为string

        // 解析 Way 的节点引用
        vector<Node> way_nodes;
        XMLElement* pNdElement = pWayElement->FirstChildElement("nd");
        while (pNdElement) {
            string ref = pNdElement->Attribute("ref"); // 存储为string
            way.node_refs.push_back(ref);
            if (nodes.find(ref) != nodes.end()) {
                way_nodes.push_back(nodes[ref]); // 存储对应的 Node
            }
            pNdElement = pNdElement->NextSiblingElement("nd");
        }

        // 解析 Way 的 Tag
        XMLElement* pWayTagElement = pWayElement->FirstChildElement("tag");
        while (pWayTagElement) {
            string key = pWayTagElement->Attribute("k");
            string value = pWayTagElement->Attribute("v");
            way.tags.push_back(make_pair(key, value));
            pWayTagElement = pWayTagElement->NextSiblingElement("tag");
        }
        
        ways[way.id] = way; // 将 Way 存储到 unordered_map 中
        
         // 将 Way 存储到对应的 Chunk 中，如果有任意一个 Node 在该 Chunk 内
        for (const auto& node : way_nodes) {
            string chunk_key = getChunkKey(node.lat, node.lon);
            chunk_map[chunk_key].ways.push_back(way);
            // break; // 一旦存入一个 Chunk，就可以跳出循环
        }

        // 将相邻节点作为邻居存储到邻接表中
        for (size_t i = 0; i < way_nodes.size(); ++i) {
            if (i > 0) { // 确保有前一个节点
                // 将前一个节点作为邻居
                double distance = calculateDistance(way_nodes[i], way_nodes[i - 1]);
                neighbors[way_nodes[i].id].push_back({way_nodes[i - 1].id, distance});
                // neighbors[way_nodes[i - 1].id].push_back({way_nodes[i].id, distance}); // 如果你想建立双向邻接表
            }
            if (i < way_nodes.size() - 1) { // 确保有后一个节点
                // 将后一个节点作为邻居
                double distance = calculateDistance(way_nodes[i], way_nodes[i + 1]);
                neighbors[way_nodes[i].id].push_back({way_nodes[i + 1].id, distance});
                // neighbors[way_nodes[i + 1].id].push_back({way_nodes[i].id, distance}); // 如果你想建立双向邻接表
            }
        }


        pWayElement = pWayElement->NextSiblingElement("way");
    }


    //  // 打印有效的 Chunk ID 并写入到文件
    // ofstream chunksFile("chunks.txt"); // 打开一个名为 chunks.txt 的文件
    // if (chunksFile.is_open()) {
    //     for (const auto& pair : chunk_map) {
    //         chunksFile << pair.first << endl; // 将 Chunk ID 写入文件
    //     }
    //     chunksFile.close(); // 关闭文件
    //     cout << "有效的 Chunk ID 已写入到 chunks.txt 文件中。" << endl;
    // } else {
    //     cout << "无法打开文件 chunks.txt 进行写入。" << endl;
    // }


    // 打印读取的 Node 和 Way（可选）
    /*
    for (const auto& pair : nodes) {
        pair.second.print();
    }
    for (const auto& pair : ways) {
        pair.second.print();
    }
    */

}

// int main() {
//     unordered_map<string, Node> nodes;
//     unordered_map<string, Way> ways;
//     // 解析 OSM 文件（确保路径正确）
//     parseOSM("data/map", nodes, ways);
//     return 0;
// }
